#pragma once

#include "boost_defs.hpp"

#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace krbn {
template <typename T>
class async_sequential_dispatcher final {
public:
  typedef std::function<void(const T&)> handler;

  async_sequential_dispatcher(const handler& handler) : handler_(handler),
                                                        exit_(false),
                                                        worker_thread_(&async_sequential_dispatcher::loop, this) {
  }

  ~async_sequential_dispatcher(void) {
    exit_ = true;
    queue_cv_.notify_one();
    worker_thread_.join();
  }

  void push_back(const std::shared_ptr<T>& entry) {
    if (entry) {
      {
        std::unique_lock<std::mutex> queue_lock(queue_mutex_);
        queue_.push_back(entry);
      }

      queue_cv_.notify_one();
    }
  }

  void wait(void) {
    while (true) {
      {
        std::unique_lock<std::mutex> queue_lock(queue_mutex_);
        if (queue_.empty()) {
          break;
        }
      }
      queue_cv_.notify_one();
    }
  }

private:
  void loop(void) {
    while (!exit_) {
      std::shared_ptr<T> entry;

      // Wait until queued

      {
        std::unique_lock<std::mutex> queue_lock(queue_mutex_);
        queue_cv_.wait(queue_lock, [this] {
          return !queue_.empty() || exit_;
        });

        if (exit_) {
          break;
        }

        entry = queue_.front();
        queue_.pop_front();
      }

      if (handler_) {
        handler_(*entry);
      }
    }
  }

  handler handler_;

  std::atomic<bool> exit_;
  std::thread worker_thread_;

  std::deque<std::shared_ptr<T>> queue_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
};
} // namespace krbn
