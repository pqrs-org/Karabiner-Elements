#pragma once

#include "logger.hpp"
#include <deque>
#include <pqrs/cf/bundle.hpp>
#include <pqrs/osx/accessibility.hpp>
#include <pqrs/osx/cg_display.hpp>
#include <pqrs/osx/cg_event.hpp>
#include <pqrs/osx/iokit_power_management.hpp>
#include <pqrs/osx/iokit_return.hpp>
#include <pqrs/osx/system_preferences.hpp>
#include <pqrs/osx/workspace.hpp>
#include <regex>

namespace krbn {
namespace console_user_server {
class software_function_handler final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  software_function_handler(void)
      : dispatcher_client(),
        check_trusted_(false) {
  }

  virtual ~software_function_handler(void) {
    detach_from_dispatcher();
  }

  void execute_software_function(const software_function& software_function) {
    if (auto v = software_function.get_if<software_function_details::cg_event_double_click>()) {
      execute_cg_event_double_click(*v);
    } else if (auto v = software_function.get_if<software_function_details::iokit_power_management_sleep_system>()) {
      execute_iokit_power_management_sleep_system(*v);
    } else if (auto v = software_function.get_if<software_function_details::open_application>()) {
      execute_open_application(*v);
    } else if (auto v = software_function.get_if<software_function_details::set_mouse_cursor_position>()) {
      execute_set_mouse_cursor_position(*v);
    }
  }

  void add_frontmost_application_history(const pqrs::osx::frontmost_application_monitor::application& application) {
    // Skip non-applications like "/System/Library/Frameworks/LocalAuthentication.framework/Support/coreautha.bundle".
    if (auto v = application.get_bundle_path()) {
      if (!pqrs::cf::bundle::application(*v)) {
        return;
      }
    }

    //
    // Remove any identical entries that already exist.
    //
    // If the same application appears multiple times in the history,
    // it causes inconvenient behavior when switching between the same application repeatedly or after an application is closed.
    // To address this, duplicates should be eliminated from the history.
    //
    // For example, consider the following application switching sequence:
    //
    // 1. Mail
    // 2. Terminal
    // 3. Safari
    // 4. Terminal
    //
    // As a user, we would expect the app two steps back to be Mail, not Terminal.
    //
    // Similarly, if Terminal is closed in this state, the focus will return to Safari.
    // If duplicates are not removed, the history will look like this:
    //
    // 1. Mail
    // 2. Terminal
    // 3. Safari
    // 4. Terminal
    // 5. Safari
    //
    // In this scenario, if we try to switch to the previous application,
    // Terminal (which is already closed) will be excluded, and the next app in the history, "3. Safari", will be selected.
    // But as a user, it would feel more natural for Mail to be selected instead.
    //
    // By removing duplicates from the history, such behavior can be achieved,
    // resulting in a smoother and more intuitive user experience.
    //

    std::erase_if(frontmost_application_history_,
                  [application](const auto& h) {
                    // `application` also includes bundle_path and pid,
                    // but we ignore those and delete history entries that match
                    // bundle_identifier and file_path, so that users can remove
                    // entries they consider to be duplicates.
                    return h.get_bundle_identifier() == application.get_bundle_identifier() &&
                           h.get_file_path() == application.get_file_path();
                  });

    //
    // Add to history
    //

    frontmost_application_history_.push_front(application);

    size_t max_size = 20;
    while (frontmost_application_history_.size() > max_size) {
      frontmost_application_history_.pop_back();
    }
  }

  void async_invoke_with_frontmost_application_history(
      std::function<void(const std::deque<pqrs::osx::frontmost_application_monitor::application>&)> function) const {
    enqueue_to_dispatcher([this, function] {
      function(frontmost_application_history_);
    });
  }

private:
  void execute_cg_event_double_click(const software_function_details::cg_event_double_click& cg_event_double_click) {
    if (!check_trusted_) {
      check_trusted_ = true;
      pqrs::osx::accessibility::is_process_trusted_with_prompt();
    }

    pqrs::osx::cg_event::mouse::post_double_click(
        pqrs::osx::cg_event::mouse::cursor_position(),
        CGMouseButton(cg_event_double_click.get_button()));
  }

  void execute_iokit_power_management_sleep_system(const software_function_details::iokit_power_management_sleep_system& iokit_power_management_sleep_system) {
    auto duration = pqrs::osx::chrono::make_absolute_time_duration(iokit_power_management_sleep_system.get_delay_milliseconds());

    enqueue_to_dispatcher(
        [] {
          auto r = pqrs::osx::iokit_power_management::sleep();
          if (!r) {
            logger::get_logger()->error("IOPMSleepSystem error: {0}", r.to_string());
          }
        },
        when_now() + pqrs::osx::chrono::make_milliseconds(duration));
  }

  void execute_open_application(const software_function_details::open_application& open_application) {
    if (auto v = open_application.get_bundle_identifier()) {
      pqrs::osx::workspace::open_application_by_bundle_identifier(*v);
    } else if (auto v = open_application.get_file_path()) {
      pqrs::osx::workspace::open_application_by_file_path(*v);
    } else if (auto v = open_application.get_frontmost_application_history_index()) {
      auto history_index = *v;

      // If the number of applications specified by frontmost_application_history_index does not exist, the oldest application will be selected.
      std::optional<pqrs::osx::frontmost_application_monitor::application> target_application;

      for (const auto& h : frontmost_application_history_) {
        // Since there are cases where the bundle paths differ even if the bundle_identifier is the same, prioritize using the bundle path.
        if (auto bundle_path = h.get_bundle_path()) {
          // Target only applications that are currently running.
          if (!pqrs::osx::workspace::application_running_by_file_path(*bundle_path)) {
            continue;
          }
          target_application = h;

        } else if (auto bundle_identifier = h.get_bundle_identifier()) {
          // Target only applications that are currently running.
          if (!pqrs::osx::workspace::application_running_by_bundle_identifier(*bundle_identifier)) {
            continue;
          }
          target_application = h;

        } else {
          continue;
        }

        if (history_index > 0) {
          --history_index;
          continue;
        }

        // Evaluating the exclusion conditions before decrementing history_index might skip the wrong entry.
        // For example, when frontmost_application_history_index is 1, the expectation is to open the immediately previous app.
        // If the current app matches an exclusion condition and is removed before history_index is decremented,
        // the logic ends up selecting the application two steps back instead of the previous one.
        // Therefore, exclusion conditions must be checked after history_index is decremented.
        if (excluded_frontmost_application_history(open_application,
                                                   h)) {
          target_application = std::nullopt;
          continue;
        }

        break;
      }

      if (target_application) {
        if (auto bundle_path = target_application->get_bundle_path()) {
          pqrs::osx::workspace::open_application_by_file_path(*bundle_path);
        } else if (auto bundle_identifier = target_application->get_bundle_identifier()) {
          pqrs::osx::workspace::open_application_by_bundle_identifier(*bundle_identifier);
        }
      }
    }
  }

  bool excluded_frontmost_application_history(const software_function_details::open_application& open_application,
                                              const pqrs::osx::frontmost_application_monitor::application& application) {
    if (auto bundle_identifier = application.get_bundle_identifier()) {
      for (const auto& regex : open_application.get_frontmost_application_history_exclusion_bundle_identifiers()) {
        if (std::regex_search(std::begin(*bundle_identifier),
                              std::end(*bundle_identifier),
                              regex.get_regex())) {
          return true;
        }
      }
    }

    if (auto file_path = application.get_file_path()) {
      for (const auto& regex : open_application.get_frontmost_application_history_exclusion_file_paths()) {
        if (std::regex_search(std::begin(*file_path),
                              std::end(*file_path),
                              regex.get_regex())) {
          return true;
        }
      }
    }

    return false;
  }

  void execute_set_mouse_cursor_position(const software_function_details::set_mouse_cursor_position& set_mouse_cursor_position) {
    if (auto target_display_id = get_target_display_id(set_mouse_cursor_position)) {
      auto local_display_point = set_mouse_cursor_position.get_point(CGDisplayBounds(*target_display_id));
      CGDisplayMoveCursorToPoint(*target_display_id, local_display_point);
    }
  }

  std::optional<CGDirectDisplayID> get_target_display_id(const software_function_details::set_mouse_cursor_position& set_mouse_cursor_position) {
    if (auto screen = set_mouse_cursor_position.get_screen()) {
      auto active_displays = pqrs::osx::cg_display::active_displays();
      if (*screen < active_displays.size()) {
        return std::optional<CGDirectDisplayID>(active_displays[*screen]);
      }
    } else {
      return pqrs::osx::cg_display::get_online_display_id_by_mouse_cursor();
    }

    return std::nullopt;
  }

private:
  bool check_trusted_;
  // Stored in order from the newest at the beginning.
  std::deque<pqrs::osx::frontmost_application_monitor::application> frontmost_application_history_;
};
} // namespace console_user_server
} // namespace krbn
