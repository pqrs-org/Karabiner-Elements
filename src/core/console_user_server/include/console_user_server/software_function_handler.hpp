#pragma once

#include "logger.hpp"
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <pqrs/osx/accessibility.hpp>
#include <pqrs/osx/cg_display.hpp>
#include <pqrs/osx/cg_event.hpp>
#include <pqrs/osx/iokit_return.hpp>
#include <pqrs/osx/system_preferences.hpp>

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
    } else if (auto v = software_function.get_if<software_function_details::set_mouse_cursor_position>()) {
      execute_set_mouse_cursor_position(*v);
    }
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
          auto fb = IOPMFindPowerManagement(MACH_PORT_NULL);
          if (fb != IO_OBJECT_NULL) {
            pqrs::osx::iokit_return r = IOPMSleepSystem(fb);
            if (!r) {
              logger::get_logger()->error("IOPMSleepSystem error: {0}", r.to_string());
            }

            IOServiceClose(fb);
          }
        },
        when_now() + pqrs::osx::chrono::make_milliseconds(duration));
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
};
} // namespace console_user_server
} // namespace krbn
