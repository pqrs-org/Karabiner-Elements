#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::hitoolbox::secure_event_input_monitor` can be used safely in a multi-threaded environment.

#include <Carbon/Carbon.h>
#include <chrono>
#include <memory>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <utility>

namespace pqrs::osx::hitoolbox {

class secure_event_input_monitor : public dispatcher::extra::dispatcher_client {
private:
  // Keep the guard first so member initialization failures also detach.
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_exception_guard_{*this};

public:
  //
  // Signals (invoked from the dispatcher thread)
  //

  nod::signal<void(bool)> secure_event_input_enabled_changed;

  //
  // Methods
  //

  secure_event_input_monitor(const secure_event_input_monitor&) = delete;
  secure_event_input_monitor& operator=(const secure_event_input_monitor&) = delete;

  secure_event_input_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
                             std::chrono::milliseconds check_interval = std::chrono::milliseconds(200))
      : dispatcher_client(std::move(weak_dispatcher)),
        check_interval_(check_interval),
        secure_event_input_enabled_(IsSecureEventInputEnabled()),
        timer_(*this) {
    dispatcher_client_constructor_exception_guard_.initialize();
  }

  ~secure_event_input_monitor() noexcept override {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      timer_.start(
          [this] {
            auto enabled = IsSecureEventInputEnabled();
            if (secure_event_input_enabled_ != enabled) {
              secure_event_input_enabled_ = enabled;
              secure_event_input_enabled_changed(secure_event_input_enabled_);
            }
          },
          check_interval_);
    });
  }

  void async_stop() {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

private:
  void stop() {
    timer_.stop();
  }

  const std::chrono::milliseconds check_interval_;
  bool secure_event_input_enabled_;
  // Construct after potentially throwing members; destruction requires detach.
  pqrs::dispatcher::extra::timer timer_;
};

} // namespace pqrs::osx::hitoolbox
