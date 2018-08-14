#pragma once

// `krbn::thread_utility::timer` can be used safely in a multi-threaded environment.
// `krbn::thread_utility::queue` can be used safely in a multi-threaded environment.

#include <chrono>
#include <functional>
#include <mutex>
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

  class queue final {
  public:
    queue(void) : exit_(false) {
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
              queue_.pop_front();
            }
          }

          if (function) {
            function();
          }
        }
      });
    }

    ~queue(void) {
      if (worker_thread_.joinable()) {
        exit_ = true;
        queue_cv_.notify_one();
        worker_thread_.join();
      }
    }

    void push_back(const std::function<void(void)>& function) {
      {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        queue_.push_back(function);
      }

      queue_cv_.notify_one();
    }

  private:
    std::thread worker_thread_;
    std::atomic<bool> exit_;

    std::deque<std::function<void(void)>> queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
  };
};
} // namespace krbn
