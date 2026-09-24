#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "dispatcher_client.hpp"
#include <cstdlib>
#include <utility>

namespace pqrs::dispatcher::extra {

// Declare this before all other members of a dispatcher client, and call
// initialize() for an empty constructor body. If member initialization throws,
// it detaches before the dispatcher_client base destructor checks for a missing detach.
// Use initialize for the constructor body so exceptions detach before member
// destruction, including timers that require their owner to be detached.
// Initialize those timers after potentially throwing members: this guard cannot
// run before an already constructed timer during member-initialization unwinding.
//
// Usage:
//
// #include <pqrs/dispatcher.hpp>
// #include <stdexcept>
//
// class client final : public pqrs::dispatcher::extra::dispatcher_client {
// private:
//   // Members initialize in declaration order. Keep the guard first.
//   pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard guard_{*this};
//
// public:
//   client(std::weak_ptr<pqrs::dispatcher::dispatcher> dispatcher, bool fail)
//       : dispatcher_client(dispatcher),
//         timer_(*this) {
//     guard_.initialize(
//         [&] {
//           // Put potentially throwing constructor-body work here.
//           if (fail) {
//             throw std::runtime_error("Client initialization failed");
//           }
//         },
//         [] {
//           // Optional cleanup on the dispatcher before members are destroyed:
//           // disconnect external signals or stop child clients here.
//           // Cleanup follows detach_from_dispatcher's availability rules
//           // and must not throw.
//         });
//   }
//
//   ~client() override {
//     // Successful construction leaves detaching to the client's destructor.
//     detach_from_dispatcher();
//   }
//
// private:
//   // Declare timers after all potentially throwing members.
//   pqrs::dispatcher::extra::timer timer_;
// };
//
// Use guard_.initialize(function) when no cleanup is needed, or
// guard_.initialize() for an empty constructor body.
// Call initialize only once. Reentrant calls and calls after success or failure abort.

class dispatcher_client_constructor_exception_guard final {
public:
  explicit dispatcher_client_constructor_exception_guard(dispatcher_client& client) noexcept
      : client_(&client) {
  }

  dispatcher_client_constructor_exception_guard(const dispatcher_client_constructor_exception_guard&) = delete;
  dispatcher_client_constructor_exception_guard& operator=(const dispatcher_client_constructor_exception_guard&) = delete;

  ~dispatcher_client_constructor_exception_guard() {
    if (client_) {
      client_->detach_from_dispatcher();
    }
  }

  void initialize() noexcept {
    begin_initialize();
    release();
  }

  template <typename Function>
  void initialize(Function&& function) {
    initialize(std::forward<Function>(function), [] {});
  }

  // Cleanup runs on the dispatcher before members are destroyed. Use it for
  // external signal connections or child clients started during construction.
  // If preparing or scheduling cleanup throws, detach without cleanup and
  // preserve the original initialization exception. Cleanup itself must not throw.
  template <typename Function, typename Cleanup>
  void initialize(Function&& function, Cleanup&& cleanup) {
    begin_initialize();

    try {
      std::forward<Function>(function)();
    } catch (...) {
      try {
        client_->detach_from_dispatcher(std::forward<Cleanup>(cleanup));
      } catch (...) {
        // Conversion/copying to std::function can fail before detaching.
        // Detach here before unwinding destroys the client's members.
        client_->detach_from_dispatcher();
      }
      release();
      throw;
    }
    release();
  }

private:
  void begin_initialize() noexcept {
    // Reject reuse without disarming the destructor's detach fallback.
    if (std::exchange(initialization_started_, true)) {
      std::abort();
    }
  }

  void release() noexcept {
    client_ = nullptr;
  }

  dispatcher_client* client_;
  bool initialization_started_ = false;
};
} // namespace pqrs::dispatcher::extra
