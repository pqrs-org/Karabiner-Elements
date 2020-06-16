#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::dispatcher::dispatcher` can be used safely in a multi-threaded environment.

#include "object_id.hpp"
#include "time_source.hpp"
#include <deque>
#include <exception>
#include <mutex>
#include <optional>
#include <pqrs/thread_wait.hpp>
#include <thread>

namespace pqrs {
namespace dispatcher {
class dispatcher final {
public:
  dispatcher(const dispatcher&) = delete;

  dispatcher(std::weak_ptr<time_source> weak_time_source) : weak_time_source_(weak_time_source),
                                                            worker_thread_id_wait_(make_thread_wait()),
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

          std::function<duration(void)> calculate_duration([this] {
            auto now = when_immediately();
            auto when = when_immediately();

            if (auto s = lock_weak_time_source()) {
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

            return duration(0);
          });

          // ----------------------------------------
          // Wait

          auto d = calculate_duration();

          if (d == duration(0)) {
            cv_.wait(lock, [this] {
              return exit_ || !queue_.empty();
            });
          } else {
            // when > now
            cv_.wait_for(lock, d, [this, &calculate_duration] {
              if (exit_) {
                return true;
              }

              if (queue_.empty()) {
                return false;
              }

              if (calculate_duration() == duration(0)) {
                return true;
              }

              return false;
            });
          }

          // ----------------------------------------
          // Check condition

          if (exit_) {
            break;
          }

          // Check `duration` again.

          d = calculate_duration();

          if (d > duration(0)) {
            continue;
          }

          // ----------------------------------------

          if (!queue_.empty()) {
            e = queue_.front();
            queue_.pop_front();
          }
        }

        if (e) {
          // Set running_function_object_id_

          {
            std::lock_guard<std::mutex> lock(running_function_object_id_mutex_);

            running_function_object_id_ = e->get_object_id_value();
          }

          running_function_object_id_cv_.notify_all();

          // Run function

          e->call_function();

          // Unset running_function_object_id_

          {
            std::lock_guard<std::mutex> lock(running_function_object_id_mutex_);

            running_function_object_id_ = std::nullopt;
          }

          running_function_object_id_cv_.notify_all();
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

  void set_weak_time_source(std::weak_ptr<time_source> value) {
    std::lock_guard<std::mutex> lock(weak_time_source_mutex_);

    weak_time_source_ = value;
  }

  std::shared_ptr<time_source> lock_weak_time_source(void) const {
    std::lock_guard<std::mutex> lock(weak_time_source_mutex_);

    return weak_time_source_.lock();
  }

  void attach(const object_id& object_id) {
    std::lock_guard<std::mutex> lock(object_ids_mutex_);

    object_ids_.insert(object_id.get());
  }

  bool detach(const object_id& object_id) {
    // Erase `object_id` from object_ids_ if exists.

    {
      std::lock_guard<std::mutex> lock(object_ids_mutex_);

      auto it = object_ids_.find(object_id.get());

      if (it == std::end(object_ids_)) {
        return false;
      }

      object_ids_.erase(it);
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
      // Wait the running function if the running function is owned by object_id.

      std::unique_lock<std::mutex> lock(running_function_object_id_mutex_);

      running_function_object_id_cv_.wait(lock, [this, &object_id] {
        return running_function_object_id_ != object_id.get();
      });
    }

    return true;
  }

  // Note:
  // Do not wait (thread::join, etc.) in `function` in order to avoid a deadlock.
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
      auto w = make_thread_wait();

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

  bool running_detached_function(void) const {
    std::lock_guard<std::mutex> lock(running_function_object_id_mutex_);

    return running_function_object_id_ == object_id_.get();
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
      // Do not call pqrs::dispatcher::terminate in the dispatcher thread.
      abort();
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

  // Note:
  // Do not wait (thread::join, etc.) in `function` in order to avoid a deadlock.
  void enqueue(const object_id& object_id,
               const std::function<void(void)>& function,
               time_point when = when_immediately()) {
    {
      std::lock_guard<std::mutex> lock(mutex_);

      auto id = object_id.get();
      auto new_entry = std::make_shared<entry>(
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
          when);

      if (when == when_internal_detached()) {
        queue_.push_front(new_entry);
      } else {
        // queue_ must be sorted by when_.

        auto it = std::find_if(std::rbegin(queue_),
                               std::rend(queue_),
                               [&](auto&& e) {
                                 return e->get_when() <= when;
                               });
        if (it == std::rend(queue_)) {
          queue_.push_front(new_entry);
        } else {
          queue_.insert(it.base(), new_entry);
        }
      }
    }

    cv_.notify_one();
  }

  void invoke(void) {
    cv_.notify_one();
  }

  static constexpr time_point when_internal_detached() {
    return time_point(duration(0));
  }

  static constexpr time_point when_immediately() {
    return time_point(duration(1));
  }

private:
  class entry final {
  public:
    entry(uint64_t object_id_value,
          const std::function<void(void)>& function,
          time_point when) : object_id_value_(object_id_value),
                             function_(function),
                             when_(when) {
    }

    uint64_t get_object_id_value(void) const {
      return object_id_value_;
    }

    time_point get_when(void) const {
      return when_;
    }

    void call_function(void) const {
      function_();
    }

  private:
    uint64_t object_id_value_;
    std::function<void(void)> function_;
    time_point when_;
  };

  std::weak_ptr<time_source> weak_time_source_;
  mutable std::mutex weak_time_source_mutex_;

  std::thread worker_thread_;
  std::thread::id worker_thread_id_;
  std::shared_ptr<thread_wait> worker_thread_id_wait_;

  std::deque<std::shared_ptr<entry>> queue_;
  bool exit_;
  std::mutex mutex_;
  std::condition_variable cv_;

  // `object_id_` is for a function after detach
  object_id object_id_;
  std::unordered_set<uint64_t> object_ids_;
  std::mutex object_ids_mutex_;

  std::optional<uint64_t> running_function_object_id_;
  mutable std::mutex running_function_object_id_mutex_;
  std::condition_variable running_function_object_id_cv_;
};
} // namespace dispatcher
} // namespace pqrs
