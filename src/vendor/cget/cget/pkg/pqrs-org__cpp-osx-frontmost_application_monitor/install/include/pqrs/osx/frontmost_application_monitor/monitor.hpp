#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::frontmost_application_monitor` can be used safely in a multi-threaded environment.

#include "application.hpp"
#include "impl/impl.h"
#include "monitor_manager.hpp"
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

  monitor(const monitor&) = delete;

  monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher) {
    std::lock_guard<std::mutex> guard(global_monitor_manager_mutex_);

    monitor_manager::get_global_monitor_manager()->insert(this);
  }

  virtual ~monitor(void) {
    std::lock_guard<std::mutex> guard(global_monitor_manager_mutex_);

    detach_from_dispatcher([this] {
      stop();
    });

    monitor_manager::get_global_monitor_manager()->erase(this);
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      pqrs_osx_frontmost_application_monitor_register(static_cpp_callback,
                                                      this);
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

private:
  void stop(void) {
    enqueue_to_dispatcher([this] {
      pqrs_osx_frontmost_application_monitor_unregister(static_cpp_callback,
                                                        this);
    });
  }

  static void static_cpp_callback(const char* bundle_identifier,
                                  const char* file_path,
                                  void* context) {
    std::lock_guard<std::mutex> guard(global_monitor_manager_mutex_);

    auto m = reinterpret_cast<monitor*>(context);
    if (m) {
      // `static_cpp_callback` may be called even after the monitor instance has been deleted,
      // so we need to check whether monitor is still alive.
      if (monitor_manager::get_global_monitor_manager()->contains(m)) {
        m->cpp_callback(bundle_identifier,
                        file_path);
      }
    }
  }

  void cpp_callback(const char* bundle_identifier,
                    const char* file_path) {
    auto application_ptr = std::make_shared<application>();
    if (bundle_identifier) {
      application_ptr->set_bundle_identifier(bundle_identifier);
    }
    if (file_path) {
      application_ptr->set_file_path(file_path);
    }

    enqueue_to_dispatcher([this, application_ptr] {
      frontmost_application_changed(application_ptr);
    });
  }

  static inline std::mutex global_monitor_manager_mutex_;
};
} // namespace frontmost_application_monitor
} // namespace osx
} // namespace pqrs
