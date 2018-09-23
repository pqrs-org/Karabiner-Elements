#pragma once

// `krbn::dispatcher::dispatcher` can be used safely in a multi-threaded environment.

#include "dispatcher/object_id.hpp"
#include "thread_utility.hpp"

namespace krbn {
namespace dispatcher {
class dispatcher final {
public:
  dispatcher(const dispatcher&) = delete;

  dispatcher(void) : exit_(false) {
    worker_thread_ = std::thread([this] {
      worker_thread_id_ = std::this_thread::get_id();
      worker_thread_id_wait_.notify();

      while (true) {
        std::function<void(void)> function;

        {
          std::unique_lock<std::mutex> lock(mutex_);

          cv_.wait(lock, [this] {
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
          std::lock_guard<std::mutex> lock(function_mutex_);

          function();
        }
      }
    });

    worker_thread_id_wait_.wait_notice();
  }

  ~dispatcher(void) {
    if (worker_thread_.joinable()) {
      logger::get_logger().error("Call `dispatcher::terminate` before destroy `dispatcher`");
      terminate();
    }
  }

  void attach(const object_id& object_id) {
    std::lock_guard<std::mutex> lock(object_ids_mutex_);

    object_ids_.insert(object_id.get());
  }

  void detach(const object_id& object_id) {
    std::lock_guard<std::mutex> lock(object_ids_mutex_);

    auto it = object_ids_.find(object_id.get());
    if (it == std::end(object_ids_)) {
      return;
    }

    object_ids_.erase(object_id.get());

    if (!is_dispatcher_thread()) {
      // Wait until current running function is finised.
      std::lock_guard<std::mutex> lock(function_mutex_);
    }
  }

  bool is_dispatcher_thread(void) const {
    return std::this_thread::get_id() == worker_thread_id_;
  }

  void terminate(void) {
    // We should separate `~dispatcher` and `terminate` to ensure dispatcher exists until all jobs are processed.
    //
    // Example:
    // ----------------------------------------
    // class example final {
    // public:
    //   example(void) : object_id_(krbn::dispatcher::make_new_object_id()) {
    //     dispatcher_ = std::make_unique<krbn::dispatcher::dispatcher>();
    //     dispatcher_->attach(object_id_);
    //
    //     dispatcher_->enqueue(
    //         object_id_,
    //         [this] {
    //           // `dispatcher_` might be nullptr if we call `terminate` before `dispatcher_ = nullptr`.
    //           dispatcher_->enqueue(
    //               object_id_,
    //               [] {
    //                 std::cout << "hello" << std::endl;
    //               });
    //         });
    //
    //     dispatcher_->terminate(); // SEGV if comment out this line
    //     dispatcher_ = nullptr;
    //   }
    //
    // private:
    //   krbn::dispatcher::object_id object_id_;
    //   std::unique_ptr<krbn::dispatcher::dispatcher> dispatcher_;
    // };
    // ----------------------------------------

    if (is_dispatcher_thread()) {
      {
        std::lock_guard<std::mutex> lock(mutex_);

        exit_ = true;
        queue_.empty();
      }

      worker_thread_.detach();

    } else {
      if (worker_thread_.joinable()) {
        {
          std::lock_guard<std::mutex> lock(mutex_);

          exit_ = true;
        }

        cv_.notify_one();
        worker_thread_.join();
      }
    }
  }

  void enqueue(const object_id& object_id,
               const std::function<void(void)>& function) {
    {
      std::lock_guard<std::mutex> lock(mutex_);

      auto id = object_id.get();
      queue_.push([this, id, function] {
        {
          std::lock_guard<std::mutex> lock(object_ids_mutex_);

          auto it = object_ids_.find(id);
          if (it == std::end(object_ids_)) {
            return;
          }
        }

        function();
      });
    }

    cv_.notify_one();
  }

private:
  std::thread worker_thread_;
  std::thread::id worker_thread_id_;
  thread_utility::wait worker_thread_id_wait_;

  std::queue<std::function<void(void)>> queue_;
  bool exit_;
  std::mutex mutex_;
  std::condition_variable cv_;

  std::unordered_set<uint64_t> object_ids_;
  std::mutex object_ids_mutex_;

  std::mutex function_mutex_;
};
} // namespace dispatcher
} // namespace krbn
