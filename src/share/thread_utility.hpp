#pragma once

// `krbn::thread_utility::wait` can be used safely in a multi-threaded environment.
// `krbn::thread_utility::timer` can be used safely in a multi-threaded environment.
// `krbn::thread_utility::dispatcher` can be used safely in a multi-threaded environment.

#include "logger.hpp"
#include <chrono>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace krbn {
class thread_utility final {
public:
  static std::thread::id get_main_thread_id(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::thread::id id = std::this_thread::get_id();
    return id;
  }

  static void register_main_thread(void) {
    get_main_thread_id();
  }

  static bool is_main_thread(void) {
    return get_main_thread_id() == std::this_thread::get_id();
  }

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

  class timer final {
  public:
    timer(const std::function<std::chrono::milliseconds(size_t count)>& interval,
          bool repeats,
          const std::function<void(void)>& function) : cancel_flag_(false),
                                                       repeats_(repeats),
                                                       count_(0) {
      thread_ = std::thread([this, interval, function] {
        do {
          // Wait
          {
            std::unique_lock<std::mutex> lock(timer_mutex_);
            timer_cv_.wait_for(lock, interval(count_), [this] {
              return cancel_flag_ == true;
            });
          }

          if (cancel_flag_) {
            return;
          }

          function();
          ++count_;
        } while (repeats_);
      });
    }

    timer(std::chrono::milliseconds interval,
          bool repeats,
          const std::function<void(void)>& function) : timer([interval](auto&& count) { return interval; },
                                                             repeats,
                                                             function) {
    }

    ~timer(void) {
      if (thread_.joinable()) {
        cancel();

        thread_.join();
      }
    }

    void wait(void) {
      if (thread_.joinable()) {
        thread_.join();
      }
    }

    void cancel(void) {
      cancel_flag_ = true;
      repeats_ = false;

      timer_cv_.notify_one();
    }

    void unset_repeats(void) {
      repeats_ = false;
    }

  private:
    std::thread thread_;
    std::atomic<bool> cancel_flag_;
    std::atomic<bool> repeats_;
    size_t count_;

    std::mutex timer_mutex_;
    std::condition_variable timer_cv_;
  };

  class dispatcher final {
  public:
    dispatcher(void) : exit_(false) {
      worker_thread_ = std::thread([this] {
        while (true) {
          std::function<void(void)> function;

          {
            std::unique_lock<std::mutex> queue_lock(queue_mutex_);
            queue_cv_.wait(queue_lock, [this] {
              return exit_ || !queue_.empty();
            });

            if (exit_ && queue_.empty()) {
              break;
            }

            if (!queue_.empty()) {
              function = queue_.front();
              queue_.pop();
            }
          }

          if (function) {
            function();
          }
        }
      });
    }

    ~dispatcher(void) {
      if (worker_thread_.joinable()) {
        logger::get_logger().error("Call `thread_utility::dispatcher::terminate` before destroy `thread_utility::dispatcher`");
        terminate();
      }
    }

    void terminate(void) {
      // We should separate `~dispatcher` and `terminate` to ensure dispatcher exists until all jobs are processed.
      //
      // Example:
      // ----------------------------------------
      // auto q = std::unique_ptr<thread_utility::dispatcher>();
      //
      // q->enqueue([q] {
      //   // `q` might be nullptr if we call `terminate` before `q = nullptr`.
      //   q->enqueue([] {
      //     std::cout << "hello" << std::endl;
      //   }
      // });
      //
      // q->terminate();
      // q = nullptr;
      // ----------------------------------------

      if (worker_thread_.joinable()) {
        exit_ = true;
        queue_cv_.notify_one();
        worker_thread_.join();
      }
    }

    void enqueue(const std::function<void(void)>& function) {
      {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        queue_.push(function);
      }

      queue_cv_.notify_one();
    }

  private:
    std::thread worker_thread_;
    std::atomic<bool> exit_;

    std::queue<std::function<void(void)>> queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
  };
};
} // namespace krbn
