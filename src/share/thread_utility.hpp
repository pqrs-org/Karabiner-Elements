#pragma once

// `krbn::thread_utility::wait` can be used safely in a multi-threaded environment.

#include "logger.hpp"
#include <chrono>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace krbn {
class thread_utility final {
public:
  class wait final {
  public:
    wait(void) : notify_(false) {
    }

    void wait_notice(void) {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] {
        return notify_;
      });
    }

    void notify(void) {
      {
        std::lock_guard<std::mutex> lock(mutex_);

        notify_ = true;
      }

      cv_.notify_one();
    }

  private:
    bool notify_;
    std::mutex mutex_;
    std::condition_variable cv_;
  };
};
} // namespace krbn
