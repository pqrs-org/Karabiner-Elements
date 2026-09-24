#pragma once

#include <pqrs/dispatcher.hpp>
#include <utility>

namespace krbn {
// Declare this before all other members of a dispatcher client, and call
// initialize() for an empty constructor body. If member initialization throws,
// it detaches before the dispatcher_client base destructor checks for a missing detach.
// Use initialize for the constructor body so exceptions detach before member
// destruction, including timers that require their owner to be detached.
// Initialize those timers after potentially throwing members: this guard cannot
// run before an already constructed timer during member-initialization unwinding.
class dispatcher_client_constructor_guard final {
public:
  explicit dispatcher_client_constructor_guard(pqrs::dispatcher::extra::dispatcher_client& client) noexcept
      : client_(&client) {
  }

  dispatcher_client_constructor_guard(const dispatcher_client_constructor_guard&) = delete;
  dispatcher_client_constructor_guard& operator=(const dispatcher_client_constructor_guard&) = delete;

  ~dispatcher_client_constructor_guard() {
    if (client_) {
      client_->detach_from_dispatcher();
    }
  }

  void initialize() noexcept {
    release();
  }

  template <typename Function>
  void initialize(Function&& function) {
    initialize(std::forward<Function>(function), [] {});
  }

  // Cleanup runs on the dispatcher before members are destroyed. Use it for
  // external signal connections or child clients started during construction.
  template <typename Function, typename Cleanup>
  void initialize(Function&& function, Cleanup&& cleanup) {
    try {
      std::forward<Function>(function)();
    } catch (...) {
      client_->detach_from_dispatcher(std::forward<Cleanup>(cleanup));
      release();
      throw;
    }
    release();
  }

private:
  void release() noexcept {
    client_ = nullptr;
  }

  pqrs::dispatcher::extra::dispatcher_client* client_;
};
} // namespace krbn
