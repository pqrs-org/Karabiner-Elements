#pragma once

// pqrs::thread_wait v2.1.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::thread_wait` can be used safely in a multi-threaded environment.

#include <atomic>
#include <memory>

namespace pqrs {
class thread_wait final {
  struct constructor_key {
  private:
    constructor_key() = default;

    friend class thread_wait;
  };

public:
  // Keep thread_wait owned by shared_ptr while another thread may call notify.
  // A waiter can return immediately after notify_ is set to true, and the object
  // must remain alive until the notifier finishes notify_.notify_all().
  static std::shared_ptr<thread_wait> make_thread_wait() {
    return std::make_shared<thread_wait>(constructor_key{});
  }

  explicit thread_wait(constructor_key) noexcept {
  }

  ~thread_wait() = default;

  thread_wait(const thread_wait&) = delete;
  thread_wait(thread_wait&&) = delete;
  thread_wait& operator=(const thread_wait&) = delete;
  thread_wait& operator=(thread_wait&&) = delete;

  void wait_notice() noexcept {
    while (!notify_.load(std::memory_order_acquire)) {
      notify_.wait(false, std::memory_order_acquire);
    }
  }

  void notify() noexcept {
    notify_.store(true, std::memory_order_release);
    notify_.notify_all();
  }

private:
  std::atomic<bool> notify_{false};
};

inline std::shared_ptr<thread_wait> make_thread_wait() {
  return thread_wait::make_thread_wait();
}
} // namespace pqrs
