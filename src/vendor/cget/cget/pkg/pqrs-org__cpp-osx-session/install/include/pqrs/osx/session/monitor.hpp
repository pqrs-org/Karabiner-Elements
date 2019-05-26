#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::session::monitor` can be used safely in a multi-threaded environment.

#include "session.hpp"
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>

namespace pqrs {
namespace osx {
namespace session {
class monitor final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(bool)> on_console_changed;

  // Methods

  monitor(const monitor&) = delete;

  monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher),
                                                                   timer_(*this) {
  }

  virtual ~monitor(void) {
    detach_from_dispatcher();
  }

  void async_start(std::chrono::milliseconds check_interval) {
    timer_.start(
        [this] {
          auto old_value = on_console_;

          auto new_value = false;
          if (auto attributes = pqrs::osx::session::find_cg_session_current_attributes()) {
            if (auto v = attributes->get_on_console()) {
              new_value = *v;
            }
          }

          if (old_value != new_value) {
            enqueue_to_dispatcher([this, new_value] {
              on_console_changed(new_value);
            });
          }

          on_console_ = new_value;
        },
        check_interval);
  }

  void async_stop(void) {
    timer_.stop();
  }

private:
  dispatcher::extra::timer timer_;
  std::optional<bool> on_console_;
};
} // namespace session
} // namespace osx
} // namespace pqrs
