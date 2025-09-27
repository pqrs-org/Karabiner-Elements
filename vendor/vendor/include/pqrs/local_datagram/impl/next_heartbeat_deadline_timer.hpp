#pragma once

// (C) Copyright Takayama Fumihiko 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "asio_helper.hpp"
#include "send_entry.hpp"
#include <algorithm>
#include <deque>
#include <filesystem>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>

namespace pqrs {
namespace local_datagram {
namespace impl {
class next_heartbeat_deadline_timer final : public dispatcher::extra::dispatcher_client {
public:
  //
  // Signals (invoked from the dispatcher thread)
  //

  nod::signal<void(void)> next_heartbeat_deadline_exceeded;

  //
  // Methods
  //

  next_heartbeat_deadline_timer(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
                                not_null_shared_ptr_t<asio::local::datagram_protocol::endpoint> sender_endpoint,
                                std::chrono::milliseconds deadline)
      : dispatcher_client(weak_dispatcher),
        sender_endpoint_(sender_endpoint),
        timer_(*this) {
    set_timer(deadline);
  }

  ~next_heartbeat_deadline_timer(void) {
    detach_from_dispatcher();
  }

  not_null_shared_ptr_t<asio::local::datagram_protocol::endpoint> get_sender_endpoint(void) const {
    return sender_endpoint_;
  }

  // `set_timer` should be called in the dispatcher thread.
  void set_timer(std::chrono::milliseconds deadline) {
    timer_count_ = 0;

    timer_.start(
        [this] {
          if (timer_count_ == 0) {
            ++timer_count_;
          } else {
            timer_.stop();
            next_heartbeat_deadline_exceeded();
          }
        },
        deadline);
  }

private:
  not_null_shared_ptr_t<asio::local::datagram_protocol::endpoint> sender_endpoint_;
  dispatcher::extra::timer timer_;
  int timer_count_;
};
} // namespace impl
} // namespace local_datagram
} // namespace pqrs
