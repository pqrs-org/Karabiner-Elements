#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "application.hpp"
#include "focused_ui_element.hpp"
#include "impl/impl.h"
#include <functional>
#include <memory>
#include <mutex>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>

namespace pqrs {
namespace osx {
namespace accessibility {
class monitor final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(pqrs::not_null_shared_ptr_t<application>)> frontmost_application_changed;
  nod::signal<void(pqrs::not_null_shared_ptr_t<focused_ui_element>)> focused_ui_element_changed;

private:
  monitor(const monitor&) = delete;

  monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher)
      : dispatcher_client(weak_dispatcher),
        last_application_(std::make_shared<application>()),
        last_focused_ui_element_(std::make_shared<focused_ui_element>()) {
    pqrs_osx_accessibility_monitor_set_callback(static_cpp_callback);
  }

public:
  virtual ~monitor() {
    pqrs_osx_accessibility_monitor_unset_callback();

    detach_from_dispatcher();
  }

  static void initialize_shared_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) {
    std::lock_guard<std::mutex> guard(shared_monitor_mutex_);

    shared_monitor_ = std::shared_ptr<monitor>(new monitor(weak_dispatcher));
  }

  static void terminate_shared_monitor() {
    std::lock_guard<std::mutex> guard(shared_monitor_mutex_);

    shared_monitor_ = nullptr;
  }

  // Return a weak_ptr instead of a shared_ptr to keep the use_count of shared_monitor_ as close to 1 as possible,
  // ensuring that terminate_shared_monitor will properly release shared_monitor_.
  static std::weak_ptr<monitor> get_shared_monitor() {
    std::lock_guard<std::mutex> guard(shared_monitor_mutex_);

    return shared_monitor_;
  }

  // Schedules retrieval of a snapshot of the current state and asynchronously invokes the signals.
  // A typical use is to call this right after setting the signals.
  void async_trigger() {
    pqrs_osx_accessibility_monitor_async_trigger();
  }

private:
  static void static_cpp_callback(int32_t force,
                                  const pqrs_osx_accessibility_snapshot* snapshot) {
    if (auto m = shared_monitor_; m) {
      m->cpp_callback(force, snapshot);
    }
  }

  static pqrs::not_null_shared_ptr_t<application> make_application(const pqrs_osx_accessibility_snapshot& snapshot) {
    auto result = std::make_shared<application>();

    if (snapshot.application_name) {
      result->set_name(snapshot.application_name);
    }
    if (snapshot.bundle_identifier) {
      result->set_bundle_identifier(snapshot.bundle_identifier);
    }
    if (snapshot.bundle_path) {
      result->set_bundle_path(snapshot.bundle_path);
    }
    if (snapshot.file_path) {
      result->set_file_path(snapshot.file_path);
    }
    if (snapshot.pid != 0) {
      result->set_pid(snapshot.pid);
    }

    return result;
  }

  static pqrs::not_null_shared_ptr_t<focused_ui_element> make_focused_ui_element(const pqrs_osx_accessibility_snapshot& snapshot) {
    auto result = std::make_shared<focused_ui_element>();

    if (snapshot.role) {
      result->set_role(snapshot.role);
    }
    if (snapshot.subrole) {
      result->set_subrole(snapshot.subrole);
    }
    if (snapshot.role_description) {
      result->set_role_description(snapshot.role_description);
    }
    if (snapshot.title) {
      result->set_title(snapshot.title);
    }
    if (snapshot.description) {
      result->set_description(snapshot.description);
    }
    if (snapshot.identifier) {
      result->set_identifier(snapshot.identifier);
    }
    if (snapshot.has_window_position != 0) {
      result->set_window_position_x(snapshot.window_position_x);
      result->set_window_position_y(snapshot.window_position_y);
    }
    if (snapshot.has_window_size != 0) {
      result->set_window_size_width(snapshot.window_size_width);
      result->set_window_size_height(snapshot.window_size_height);
    }

    return result;
  }

  void cpp_callback(int32_t force,
                    const pqrs_osx_accessibility_snapshot* snapshot) {
    if (!snapshot) {
      return;
    }

    auto current_application = make_application(*snapshot);
    auto current_focused_ui_element = make_focused_ui_element(*snapshot);

    enqueue_to_dispatcher([this, force, current_application, current_focused_ui_element] {
      // `force` is non-zero when async_trigger() explicitly requests callbacks even if the snapshot is unchanged.
      if (force != 0 || *last_application_ != *current_application) {
        last_application_ = current_application;
        frontmost_application_changed(current_application);
      }

      if (force != 0 || *last_focused_ui_element_ != *current_focused_ui_element) {
        last_focused_ui_element_ = current_focused_ui_element;
        focused_ui_element_changed(current_focused_ui_element);
      }
    });
  }

  static inline std::shared_ptr<monitor> shared_monitor_;
  static inline std::mutex shared_monitor_mutex_;

  pqrs::not_null_shared_ptr_t<application> last_application_;
  pqrs::not_null_shared_ptr_t<focused_ui_element> last_focused_ui_element_;
};
} // namespace accessibility
} // namespace osx
} // namespace pqrs
