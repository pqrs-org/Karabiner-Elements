#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::dispatcher::extra::debounced_task` can be used safely in a multi-threaded environment.

#include "dispatcher_client.hpp"
#include <cstdint>
#include <functional>
#include <utility>

namespace pqrs::dispatcher::extra {

// Usage Note:
//
// We must not destroy a debounced_task before dispatcher_client is detached.
// (It causes that dispatcher might access the released debounced_task.)
// debounced_task calls `abort` if you destroy debounced_task while
// dispatcher_client is attached in order to avoid the above case.

class debounced_task final {
public:
  explicit debounced_task(dispatcher_client& dispatcher_client)
      : dispatcher_client_(dispatcher_client) {
  }

  ~debounced_task() {
    if (dispatcher_client_.attached()) {
      // Do not release debounced_task before `dispatcher_client_` is detached.
      abort();
    }
  }

  bool debounce_at(std::function<void()> function,
                   time_point when) {
    return dispatcher_client_.enqueue_to_dispatcher([this, function = std::move(function), when] mutable {
      debounce_in_dispatcher(std::move(function), when);
    });
  }

  bool debounce_after(std::function<void()> function,
                      duration delay) {
    return debounce_at(std::move(function), dispatcher_client_.when_now() + delay);
  }

  void cancel() {
    dispatcher_client_.enqueue_to_dispatcher([this] {
      ++generation_;
    });
  }

private:
  // This method is executed in the dispatcher thread.
  bool debounce_in_dispatcher(std::function<void()> function,
                              time_point when) {
    auto generation = ++generation_;

    if (!dispatcher_client_.enqueue_to_dispatcher(
            [this, function = std::move(function), generation] {
              if (generation_ != generation) {
                return;
              }

              function();
            },
            when)) {
      ++generation_;
      return false;
    }

    return true;
  }

  dispatcher_client& dispatcher_client_;
  uint64_t generation_ = 0;
};
} // namespace pqrs::dispatcher::extra
