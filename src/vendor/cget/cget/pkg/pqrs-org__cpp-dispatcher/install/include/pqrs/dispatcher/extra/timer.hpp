#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::dispatcher::extra::timer` can be used safely in a multi-threaded environment.

#include "dispatcher_client.hpp"

namespace pqrs {
namespace dispatcher {
namespace extra {

// Usage Note:
//
// We must not destroy a timer before dispatcher_client is detached.
// (It causes that dispatcher might access the released timer.)
// timer calls `abort` if you destroy timer while
// dispatcher_client is attached in order to avoid the above case.

class timer final {
public:
  timer(dispatcher_client& dispatcher_client) : dispatcher_client_(dispatcher_client),
                                                current_function_id_(0),
                                                interval_(0) {
  }

  ~timer(void) {
    if (dispatcher_client_.attached()) {
      // Do not release timer before `dispatcher_client_` is detached.
      abort();
    }
  }

  void start(std::function<void(void)> function,
             duration interval) {
    dispatcher_client_.enqueue_to_dispatcher([this, function, interval] {
      ++current_function_id_;
      function_ = function;
      interval_ = interval;

      call_function(current_function_id_);
    });
  }

  void stop(void) {
    dispatcher_client_.enqueue_to_dispatcher([this] {
      ++current_function_id_;
      function_ = nullptr;
      interval_ = duration(0);
    });
  }

private:
  // This method is executed in the dispatcher thread.
  void call_function(int function_id) {
    if (current_function_id_ != function_id) {
      return;
    }

    if (function_) {
      // We should capture function_ to call proper function even if function_ is updated in `start` or `stop` method.
      auto f = function_;

      // The `function_` call must be wrapped in enqueue_to_dispatcher in order to avoid heap-use-after-free when the timer itself is destroyed in `function_`.
      dispatcher_client_.enqueue_to_dispatcher([f] {
        f();
      });
    }

    dispatcher_client_.enqueue_to_dispatcher(
        [this, function_id] {
          call_function(function_id);
        },
        dispatcher_client_.when_now() + interval_);
  }

  dispatcher_client& dispatcher_client_;
  int current_function_id_;
  std::function<void(void)> function_;
  duration interval_;
};
} // namespace extra
} // namespace dispatcher
} // namespace pqrs
