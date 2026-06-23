#pragma once

// pqrs::thread_wait v2.2.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::thread_wait` can be used safely in a multi-threaded environment.

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>

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
    std::unique_lock<std::mutex> lock(mutex_);
    condition_variable_.wait(lock, [this] {
      return notify_.load(std::memory_order_acquire);
    });
  }

  // Returns true if notified, or false if the timeout expires.
  template <class Rep, class Period>
  bool wait_notice_for(const std::chrono::duration<Rep, Period>& timeout_duration) noexcept {
    std::unique_lock<std::mutex> lock(mutex_);
    return condition_variable_.wait_for(lock,
                                        timeout_duration,
                                        [this] {
                                          return notify_.load(std::memory_order_acquire);
                                        });
  }

  void notify() noexcept {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      notify_.store(true, std::memory_order_release);
    }

    condition_variable_.notify_all();
  }

private:
  std::atomic<bool> notify_{false};
  std::mutex mutex_;
  std::condition_variable condition_variable_;
};

inline std::shared_ptr<thread_wait> make_thread_wait() {
  return thread_wait::make_thread_wait();
}
} // namespace pqrs
