#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::frontmost_application_monitor` can be used safely in a multi-threaded environment.

#include "application.hpp"
#include "impl/impl.h"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>

namespace pqrs {
namespace osx {
namespace frontmost_application_monitor {
class monitor final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(std::shared_ptr<application>)> frontmost_application_changed;

  // Methods

private:
  monitor(const monitor&) = delete;

  monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher) {
    pqrs_osx_frontmost_application_monitor_set_callback(static_cpp_callback);
  }

public:
  virtual ~monitor(void) {
    pqrs_osx_frontmost_application_monitor_unset_callback();

    detach_from_dispatcher();
  }

  static void initialize_shared_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) {
    std::lock_guard<std::mutex> guard(mutex_);

    shared_monitor_ = std::shared_ptr<monitor>(new monitor(weak_dispatcher));
  }

  static void terminate_shared_monitor(void) {
    std::lock_guard<std::mutex> guard(mutex_);

    shared_monitor_ = nullptr;
  }

  // Return a weak_ptr instead of a shared_ptr to keep the use_count of shared_monitor_ as close to 1 as possible,
  // ensuring that terminate_shared_monitor will properly release shared_monitor_.
  static std::weak_ptr<monitor> get_shared_monitor(void) {
    std::lock_guard<std::mutex> guard(mutex_);

    return shared_monitor_;
  }

  void trigger(void) {
    pqrs_osx_frontmost_application_monitor_trigger();
  }

private:
  static void static_cpp_callback(const char* bundle_identifier,
                                  const char* bundle_path,
                                  const char* file_path,
                                  pid_t pid) {
    auto m = shared_monitor_;
    if (m) {
      m->cpp_callback(bundle_identifier,
                      bundle_path,
                      file_path,
                      pid);
    }
  }

  void cpp_callback(const char* bundle_identifier,
                    const char* bundle_path,
                    const char* file_path,
                    pid_t pid) {
    auto application_ptr = std::make_shared<application>();
    if (bundle_identifier) {
      application_ptr->set_bundle_identifier(bundle_identifier);
    }
    if (bundle_path) {
      application_ptr->set_bundle_path(bundle_path);
    }
    if (file_path) {
      application_ptr->set_file_path(file_path);
    }
    if (pid) {
      application_ptr->set_pid(pid);
    }

    enqueue_to_dispatcher([this, application_ptr] {
      frontmost_application_changed(application_ptr);
    });
  }

  static inline std::shared_ptr<monitor> shared_monitor_;
  static inline std::mutex mutex_;
};
} // namespace frontmost_application_monitor
} // namespace osx
} // namespace pqrs
