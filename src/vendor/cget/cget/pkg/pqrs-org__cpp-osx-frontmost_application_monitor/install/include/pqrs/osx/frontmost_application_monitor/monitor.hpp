#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::frontmost_application_monitor` can be used safely in a multi-threaded environment.

#include "application.hpp"
#include "impl/objc.h"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>

namespace pqrs {
namespace osx {
namespace frontmost_application_monitor {
class monitor final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(std::shared_ptr<application>)> frontmost_application_changed;

  // Methods

  monitor(const monitor&) = delete;

  monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher),
                                                                   monitor_(nullptr) {
  }

  virtual ~monitor(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      if (monitor_) {
        return;
      }

      pqrs_osx_frontmost_application_monitor_initialize(&monitor_,
                                                        static_cpp_callback,
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
    if (!monitor_) {
      return;
    }

    pqrs_osx_frontmost_application_monitor_terminate(&monitor_);
    monitor_ = nullptr;
  }

  static void static_cpp_callback(const char* bundle_identifier,
                                  const char* file_path,
                                  void* context) {
    auto m = reinterpret_cast<monitor*>(context);
    if (m) {
      m->cpp_callback(bundle_identifier, file_path);
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

  pqrs_osx_frontmost_application_monitor_objc* monitor_;
};
} // namespace frontmost_application_monitor
} // namespace osx
} // namespace pqrs
