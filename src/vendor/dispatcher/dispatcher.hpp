#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::dispatcher::dispatcher` can be used safely in a multi-threaded environment.

#include "dispatcher/object_id.hpp"
#include "dispatcher/wait.hpp"
#include "time_source.hpp"
#include <deque>
#include <exception>
#include <mutex>
#include <thread>

namespace pqrs {
namespace dispatcher {
class dispatcher final {
public:
  dispatcher(const dispatcher&) = delete;

  dispatcher(std::weak_ptr<time_source> weak_time_source) : weak_time_source_(weak_time_source),
                                                            worker_thread_id_wait_(make_wait()),
                                                            exit_(false),
                                                            object_id_(make_new_object_id()) {
    worker_thread_ = std::thread([this] {
      worker_thread_id_ = std::this_thread::get_id();
      worker_thread_id_wait_->notify();

      while (true) {
        std::shared_ptr<entry> e;

        {
          std::unique_lock<std::mutex> lock(mutex_);

          // ----------------------------------------

          std::function<std::chrono::milliseconds(void)> calculate_duration([this] {
            std::chrono::milliseconds now = when_immediately();
            std::chrono::milliseconds when = when_immediately();

            if (auto s = weak_time_source_.lock()) {
              auto n = s->now();
              if (now < n) {
                now = n;
              }
            }

            if (!queue_.empty()) {
              when = queue_.front()->get_when();
            }

            if (now < when) {
              return when - now;
            }

            return std::chrono::milliseconds(0);
          });

          // ----------------------------------------
          // Wait

          auto duration = calculate_duration();

          if (duration == std::chrono::milliseconds(0)) {
            cv_.wait(lock, [this] {
              return exit_ || !queue_.empty();
            });
          } else {
            // when > now
            cv_.wait_for(lock, duration, [this, &calculate_duration] {
              if (exit_) {
                return true;
              }

              if (queue_.empty()) {
                return false;
              }

              if (calculate_duration() == std::chrono::milliseconds(0)) {
                return true;
              }

              return false;
            });
          }

          // ----------------------------------------
          // Check condition

          if (exit_ && queue_.empty()) {
            break;
          }

          // Check `duration` again.

          duration = calculate_duration();

          if (duration > std::chrono::milliseconds(0)) {
            continue;
          }

          // ----------------------------------------

          if (!queue_.empty()) {
            e = queue_.front();
            queue_.pop_front();
          }
        }

        if (e) {
          std::lock_guard<std::mutex> lock(function_mutex_);

          e->call_function();
        }
      }
    });

    worker_thread_id_wait_->wait_notice();

    attach(object_id_);
  }

  ~dispatcher(void) {
    if (worker_thread_.joinable()) {
      terminate();
    }
  }

  std::weak_ptr<time_source> get_weak_time_source(void) {
    return weak_time_source_;
  }

  void attach(const object_id& object_id) {
    std::lock_guard<std::mutex> lock(object_ids_mutex_);

    object_ids_.insert(object_id.get());
  }

  bool detach(const object_id& object_id) {
    // Erase `object_id` from object_ids_ if exists.

    {
      std::lock_guard<std::mutex> lock(object_ids_mutex_);

      if (object_ids_.find(object_id.get()) == std::end(object_ids_)) {
        return false;
      }

      object_ids_.erase(object_id.get());
    }

    // Erase entries

    {
      std::lock_guard<std::mutex> lock(mutex_);

      queue_.erase(std::remove_if(std::begin(queue_),
                                  std::end(queue_),
                                  [&](auto&& e) {
                                    return e->get_object_id_value() == object_id.get();
                                  }),
                   std::end(queue_));
    }

    if (!dispatcher_thread()) {
      // Wait until current running function is finised.
      std::lock_guard<std::mutex> lock(function_mutex_);
    }

    return true;
  }

  void detach(const object_id& object_id,
              const std::function<void(void)>& function) {
    if (!detach(object_id)) {
      return;
    }

    // Skip `function` if dispatcher is terminating or already terminated.

    {
      std::lock_guard<std::mutex> lock(mutex_);

      if (exit_) {
        return;
      }
    }

    // Execute function

    if (dispatcher_thread()) {
      function();
    } else {
      auto w = make_wait();

      // Run detached function with dispatcher's object_id.
      // (`object_id` in arguments is already detached.)

      enqueue(object_id_,
              [w, &function] {
                function();
                w->notify();
              },
              when_internal_detached());

      w->wait_notice();
    }
  }

  bool attached(const object_id& object_id) {
    std::lock_guard<std::mutex> lock(object_ids_mutex_);

    return object_ids_.find(object_id.get()) != std::end(object_ids_);
  }

  bool dispatcher_thread(void) const {
    return std::this_thread::get_id() == worker_thread_id_;
  }

  void terminate(void) {
    // We should separate `~dispatcher` and `terminate` to ensure dispatcher exists until all jobs are processed.
    //
    // Example:
    // ----------------------------------------
    // class example final {
    // public:
    //   example(void) : object_id_(pqrs::dispatcher::make_new_object_id()) {
    //     dispatcher_ = std::make_unique<pqrs::dispatcher::dispatcher>();
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
    //   pqrs::dispatcher::object_id object_id_;
    //   std::unique_ptr<pqrs::dispatcher::dispatcher> dispatcher_;
    // };
    // ----------------------------------------

    if (dispatcher_thread()) {
      throw std::logic_error("Do not call pqrs::dispatcher::terminate in the dispatcher thread.");
    }

    if (worker_thread_.joinable()) {
      {
        std::lock_guard<std::mutex> lock(mutex_);

        exit_ = true;
      }

      cv_.notify_one();
      worker_thread_.join();
    }
  }

  void enqueue(const object_id& object_id,
               const std::function<void(void)>& function,
               std::chrono::milliseconds when = when_immediately()) {
    {
      std::lock_guard<std::mutex> lock(mutex_);

      auto id = object_id.get();
      queue_.push_back(std::make_shared<entry>(
          id,
          [this, id, function] {
            // Check `id` is attached.

            {
              std::lock_guard<std::mutex> lock(object_ids_mutex_);

              if (object_ids_.find(id) == std::end(object_ids_)) {
                return;
              }
            }

            // Execute `function`.

            function();
          },
          when));

      std::stable_sort(std::begin(queue_),
                       std::end(queue_),
                       [](auto& a, auto& b) {
                         return a->get_when() < b->get_when();
                       });
    }

    cv_.notify_one();
  }

  void invoke(void) {
    cv_.notify_one();
  }

  static constexpr std::chrono::milliseconds when_internal_detached() {
    return std::chrono::milliseconds(0);
  }

  static constexpr std::chrono::milliseconds when_immediately() {
    return std::chrono::milliseconds(1);
  }

private:
  class entry final {
  public:
    entry(uint64_t object_id_value,
          const std::function<void(void)>& function,
          std::chrono::milliseconds when) : object_id_value_(object_id_value),
                                            function_(function),
                                            when_(when) {
    }

    uint64_t get_object_id_value(void) const {
      return object_id_value_;
    }

    std::chrono::milliseconds get_when(void) const {
      return when_;
    }

    void call_function(void) const {
      function_();
    }

  private:
    uint64_t object_id_value_;
    std::function<void(void)> function_;
    std::chrono::milliseconds when_;
  };

  std::weak_ptr<time_source> weak_time_source_;

  std::thread worker_thread_;
  std::thread::id worker_thread_id_;
  std::shared_ptr<wait> worker_thread_id_wait_;

  std::deque<std::shared_ptr<entry>> queue_;
  bool exit_;
  std::mutex mutex_;
  std::condition_variable cv_;

  object_id object_id_;
  std::unordered_set<uint64_t> object_ids_;
  std::mutex object_ids_mutex_;

  std::mutex function_mutex_;
};
} // namespace dispatcher
} // namespace pqrs
