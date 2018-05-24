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
                                                        exit_(false) {
    refresh_worker_thread();
  }

  ~async_sequential_dispatcher(void) {
    {
      std::unique_lock<std::mutex> queue_lock(queue_mutex_);
      queue_.clear();
    }

    if (worker_thread_) {
      exit_ = true;
      queue_cv_.notify_one();
      worker_thread_->join();
      worker_thread_ = nullptr;
    }
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
    if (worker_thread_) {
      exit_ = true;
      queue_cv_.notify_one();
      worker_thread_->join();
      worker_thread_ = nullptr;
    }

    refresh_worker_thread();
  }

private:
  void refresh_worker_thread(void) {
    if (worker_thread_) {
      wait();
    }

    exit_ = false;
    worker_thread_ = std::make_unique<std::thread>(&async_sequential_dispatcher::loop, this);
  }

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

    // Handle remaining entries

    {
      std::unique_lock<std::mutex> queue_lock(queue_mutex_);
      while (!queue_.empty()) {
        if (handler_) {
          handler_(*(queue_.front()));
          queue_.pop_front();
        }
      }
    }
  }

  handler handler_;

  std::atomic<bool> exit_;
  std::unique_ptr<std::thread> worker_thread_;

  std::deque<std::shared_ptr<T>> queue_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
};
} // namespace krbn
