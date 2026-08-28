#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "application.hpp"
#include "focused_ui_element.hpp"
#include "impl/impl.h"
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>

namespace pqrs::osx::accessibility {
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
  }

  void register_callback() {
    pqrs_osx_accessibility_monitor_set_callback(static_cpp_callback);
  }

  void unregister_callback_and_detach() {
    pqrs_osx_accessibility_monitor_unset_callback();

    detach_from_dispatcher();
  }

public:
  ~monitor() override = default;

  // initialize_shared_monitor and terminate_shared_monitor must be called
  // serially during application lifecycle transitions.
  // Every successful initialization must be paired with termination before
  // the shared dispatcher is terminated or the last monitor reference is
  // released.
  // External callers must not retrieve or use the shared monitor until
  // initialize_shared_monitor returns.
  // They must also stop using any previously retrieved monitor before
  // terminate_shared_monitor begins. Keeping a shared_ptr alive preserves the
  // C++ object, but operations such as trigger target the process-wide Swift
  // monitor and must not cross monitor lifecycle boundaries.
  //
  // terminate_shared_monitor may synchronously wait for a running dispatcher
  // callback to finish. Signal handlers must therefore not synchronously wait
  // for the thread that calls terminate_shared_monitor. Also, do not call
  // termination directly from this monitor's signal handlers because that can
  // destroy the monitor while its dispatcher callback is still running.
  // Scheduling termination as a separate dispatcher task avoids both issues.
  static void initialize_shared_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) {
    auto m = std::shared_ptr<monitor>(new monitor(weak_dispatcher));

    {
      std::lock_guard<std::mutex> guard(shared_monitor_mutex_);

      if (shared_monitor_) {
        std::abort();
      }

      shared_monitor_ = m;
    }

    // register_callback must be called after assigning shared_monitor_ so that
    // a callback delivered immediately during registration can retrieve the
    // monitor. Swift currently schedules the initial refresh asynchronously,
    // but that is an implementation detail and must not be relied upon by
    // moving register_callback into the constructor.
    //
    // Registering the callback synchronously enters Swift on MainActor. Do not
    // hold shared_monitor_mutex_ here because the callback can re-enter
    // static_cpp_callback and acquire it via get_shared_monitor().
    m->register_callback();
  }

  static void terminate_shared_monitor() {
    std::shared_ptr<monitor> m;

    {
      std::lock_guard<std::mutex> guard(shared_monitor_mutex_);

      // Stop new Swift callbacks from retrieving the monitor before detaching
      // it from Swift and the dispatcher.
      m = std::move(shared_monitor_);
    }

    if (m) {
      // Perform synchronous cleanup while this local shared_ptr keeps the
      // monitor alive. Cleanup must not be deferred to the destructor because
      // another shared_ptr could make the destructor run later on an arbitrary
      // thread.
      m->unregister_callback_and_detach();
    }
  }

  // Return a weak_ptr so that retrieving the shared monitor does not by itself
  // extend its lifetime. A caller may temporarily lock the weak_ptr, but
  // terminate_shared_monitor explicitly unregisters and detaches the monitor
  // even while such shared_ptr instances remain alive.
  [[nodiscard]] static std::weak_ptr<monitor> get_shared_monitor() {
    std::lock_guard<std::mutex> guard(shared_monitor_mutex_);

    return shared_monitor_;
  }

  // Retrieves a snapshot of the current state, then asynchronously invokes the signals.
  // A typical use is to call this right after setting the signals.
  void trigger() {
    pqrs_osx_accessibility_monitor_trigger();
  }

private:
  static void static_cpp_callback(int32_t force,
                                  const pqrs_osx_accessibility_snapshot* snapshot) {
    if (auto m = get_shared_monitor().lock()) {
      m->cpp_callback(force, snapshot);
    }
  }

  static pqrs::not_null_shared_ptr_t<application> make_application(const pqrs_osx_accessibility_snapshot& snapshot) {
    pqrs::not_null_shared_ptr_t<application> result(std::make_shared<application>());

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
    switch (snapshot.application_detection_source) {
      case 1:
        result->set_detection_source(application::detection_source::workspace);
        break;
      case 2:
        result->set_detection_source(application::detection_source::ax_observer);
        break;
      default:
        result->set_detection_source(application::detection_source::none);
        break;
    }

    return result;
  }

  static pqrs::not_null_shared_ptr_t<focused_ui_element> make_focused_ui_element(const pqrs_osx_accessibility_snapshot& snapshot) {
    pqrs::not_null_shared_ptr_t<focused_ui_element> result(std::make_shared<focused_ui_element>());

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
    if (snapshot.window_title) {
      result->set_window_title(snapshot.window_title);
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

    const auto forced = force != 0;
    auto current_application = make_application(*snapshot);
    auto current_focused_ui_element = make_focused_ui_element(*snapshot);

    enqueue_to_dispatcher([this, forced, current_application, current_focused_ui_element] {
      // `force` is non-zero when trigger() explicitly requests callbacks even if the snapshot is unchanged.
      if (forced || *last_application_ != *current_application) {
        last_application_ = current_application;
        frontmost_application_changed(current_application);
      }

      if (forced || *last_focused_ui_element_ != *current_focused_ui_element) {
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
} // namespace pqrs::osx::accessibility
