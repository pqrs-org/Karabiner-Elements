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

  // Retrieves a snapshot of the current state and invokes the signals.
  // A typical use is to call this right after setting the signals.
  void trigger() {
    pqrs_osx_accessibility_monitor_trigger();
  }

private:
  static void static_cpp_callback(int32_t force,
                                  const char* application_name,
                                  const char* bundle_identifier,
                                  const char* bundle_path,
                                  const char* file_path,
                                  pid_t pid,
                                  const char* role,
                                  const char* subrole,
                                  const char* role_description,
                                  const char* title,
                                  const char* description,
                                  const char* identifier) {
    if (auto m = shared_monitor_; m) {
      m->cpp_callback(force,
                      application_name,
                      bundle_identifier,
                      bundle_path,
                      file_path,
                      pid,
                      role,
                      subrole,
                      role_description,
                      title,
                      description,
                      identifier);
    }
  }

  static pqrs::not_null_shared_ptr_t<application> make_application(const char* application_name,
                                                                   const char* bundle_identifier,
                                                                   const char* bundle_path,
                                                                   const char* file_path,
                                                                   pid_t pid) {
    auto result = std::make_shared<application>();

    if (application_name) {
      result->set_name(application_name);
    }
    if (bundle_identifier) {
      result->set_bundle_identifier(bundle_identifier);
    }
    if (bundle_path) {
      result->set_bundle_path(bundle_path);
    }
    if (file_path) {
      result->set_file_path(file_path);
    }
    if (pid != 0) {
      result->set_pid(pid);
    }

    return result;
  }

  static pqrs::not_null_shared_ptr_t<focused_ui_element> make_focused_ui_element(const char* role,
                                                                                 const char* subrole,
                                                                                 const char* role_description,
                                                                                 const char* title,
                                                                                 const char* description,
                                                                                 const char* identifier) {
    auto result = std::make_shared<focused_ui_element>();

    if (role) {
      result->set_role(role);
    }
    if (subrole) {
      result->set_subrole(subrole);
    }
    if (role_description) {
      result->set_role_description(role_description);
    }
    if (title) {
      result->set_title(title);
    }
    if (description) {
      result->set_description(description);
    }
    if (identifier) {
      result->set_identifier(identifier);
    }

    return result;
  }

  void cpp_callback(int32_t force,
                    const char* application_name,
                    const char* bundle_identifier,
                    const char* bundle_path,
                    const char* file_path,
                    pid_t pid,
                    const char* role,
                    const char* subrole,
                    const char* role_description,
                    const char* title,
                    const char* description,
                    const char* identifier) {
    auto current_application = make_application(application_name,
                                                bundle_identifier,
                                                bundle_path,
                                                file_path,
                                                pid);

    auto current_focused_ui_element = make_focused_ui_element(role,
                                                              subrole,
                                                              role_description,
                                                              title,
                                                              description,
                                                              identifier);

    enqueue_to_dispatcher([this, force, current_application, current_focused_ui_element] {
      // `force` is non-zero when trigger() explicitly requests callbacks even if the snapshot is unchanged.
      if (force != 0 || last_application_ != current_application) {
        last_application_ = current_application;
        frontmost_application_changed(current_application);
      }

      if (force != 0 || last_focused_ui_element_ != current_focused_ui_element) {
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
