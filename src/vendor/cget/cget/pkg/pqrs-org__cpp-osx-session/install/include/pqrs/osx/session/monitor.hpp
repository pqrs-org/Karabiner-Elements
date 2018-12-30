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

  nod::signal<void(std::optional<uid_t>)> console_user_id_changed;

  // Methods

  monitor(const monitor&) = delete;

  monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher),
                                                                   timer_(*this) {
  }

  virtual ~monitor(void) {
    detach_from_dispatcher([] {
    });
  }

  void async_start(std::chrono::milliseconds check_interval) {
    timer_.start(
        [this] {
          auto u = find_console_user_id();
          if (!console_user_id_ || *console_user_id_ != u) {
            console_user_id_ = std::make_unique<std::optional<uid_t>>(u);

            enqueue_to_dispatcher([this, u] {
              console_user_id_changed(u);
            });
          }
        },
        check_interval);
  }

  void async_stop(void) {
    timer_.stop();
  }

private:
  dispatcher::extra::timer timer_;
  std::unique_ptr<std::optional<uid_t>> console_user_id_;
};
} // namespace session
} // namespace osx
} // namespace pqrs
