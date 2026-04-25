#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::dispatcher::dispatcher` can be used safely in a multi-threaded environment.

#include "object_id.hpp"
#include "time_source.hpp"
#include <algorithm>
#include <deque>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <pqrs/thread_wait.hpp>
#include <thread>

namespace pqrs::dispatcher {
class dispatcher final {
public:
  dispatcher(const dispatcher&) = delete;

  dispatcher(std::weak_ptr<time_source> weak_time_source) : weak_time_source_(weak_time_source),
                                                            worker_thread_id_wait_(make_thread_wait()),
                                                            object_id_(make_new_object_id()) {
    worker_thread_ = std::thread([this] {
      worker_thread_id_ = std::this_thread::get_id();
      worker_thread_id_wait_->notify();

      while (true) {
        std::shared_ptr<entry> e;

        {
          std::unique_lock<std::mutex> lock(mutex_);

          // ----------------------------------------

          const auto calculate_duration = [this] {
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

            return duration::zero();
          };

          // ----------------------------------------
          // Wait

          auto d = calculate_duration();

          if (d == duration::zero()) {
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

              if (calculate_duration() == duration::zero()) {
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

          if (d > duration::zero()) {
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

  ~dispatcher() {
    if (worker_thread_.joinable()) {
      terminate();
    }
  }

  void set_weak_time_source(std::weak_ptr<time_source> value) {
    std::lock_guard<std::mutex> lock(weak_time_source_mutex_);

    weak_time_source_ = value;
  }

  std::shared_ptr<time_source> lock_weak_time_source() const {
    std::lock_guard<std::mutex> lock(weak_time_source_mutex_);

    return weak_time_source_.lock();
  }

  // Returns false if the dispatcher is terminating or already terminated.
  bool attach(const object_id& object_id) {
    std::scoped_lock lock(object_ids_mutex_, mutex_);

    if (exit_) {
      return false;
    }

    object_ids_.insert(object_id.get());
    return true;
  }

  bool detach(const object_id& object_id) {
    // Erase `object_id` from object_ids_ if exists.

    {
      std::scoped_lock lock(object_ids_mutex_, mutex_);

      if (!object_ids_.contains(object_id.get())) {
        return false;
      }

      object_ids_.erase(object_id.get());

      std::erase_if(queue_, [&](const auto& e) {
        return e->get_object_id_value() == object_id.get();
      });
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
  // - `function` is intended for cleanup and `detach` does not return until `function` is finished.
  // - Do not wait (thread::join, etc.) in `function` in order to avoid a deadlock.
  // - If `detach` is called from the dispatcher thread, `function` is executed inline.
  //   In that case, `function` might still run even if `terminate()` starts concurrently after `detach` begins.
  void detach(const object_id& object_id,
              std::function<void()> function) {
    if (!detach(object_id)) {
      return;
    }

    //
    // Execute function
    //

    if (dispatcher_thread()) {
      function();
    } else {
      auto w = make_thread_wait();

      // Run detached function with dispatcher's object_id.
      // (`object_id` in arguments is already detached.)

      if (enqueue(
              object_id_,
              [w, function] {
                function();
                w->notify();
              },
              when_internal_detached())) {
        w->wait_notice();
      }
    }
  }

  bool attached(const object_id& object_id) {
    std::lock_guard<std::mutex> lock(object_ids_mutex_);

    return object_ids_.contains(object_id.get());
  }

  bool dispatcher_thread() const {
    return std::this_thread::get_id() == worker_thread_id_;
  }

  bool running_detached_function() const {
    std::lock_guard<std::mutex> lock(running_function_object_id_mutex_);

    return running_function_object_id_ == object_id_.get();
  }

  void terminate() {
    // We should separate `~dispatcher` and `terminate` to ensure dispatcher exists until all jobs are processed.
    //
    // Example:
    // ----------------------------------------
    // class example final {
    // public:
    //   example() : object_id_(pqrs::dispatcher::make_new_object_id()) {
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

      cv_.notify_all();
      worker_thread_.join();
    }
  }

  // Note:
  // - Returns false if the dispatcher is terminating, already terminated, or `object_id` is not attached.
  // - Do not wait (thread::join, etc.) in `function` in order to avoid a deadlock.
  bool enqueue(const object_id& object_id,
               std::function<void()> function,
               time_point when = when_immediately()) {
    auto id = object_id.get();

    {
      std::scoped_lock lock(object_ids_mutex_, mutex_);

      if (!object_ids_.contains(id)) {
        return false;
      }

      if (exit_) {
        return false;
      }

      auto new_entry = std::make_shared<entry>(
          id,
          [this, id, function] {
            // Check `id` is attached.

            {
              std::lock_guard<std::mutex> lock(object_ids_mutex_);

              if (!object_ids_.contains(id)) {
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
        auto it = std::ranges::upper_bound(
            queue_,
            when,
            std::less<>{},
            [](const auto& e) {
              return e->get_when();
            });
        queue_.insert(it, new_entry);
      }
    }

    cv_.notify_all();
    return true;
  }

  void invoke() {
    cv_.notify_all();
  }

  static constexpr time_point when_internal_detached() {
    return time_point(duration::zero());
  }

  static constexpr time_point when_immediately() {
    return time_point(duration(1));
  }

private:
  class entry final {
  public:
    entry(uint64_t object_id_value,
          std::function<void()> function,
          time_point when) : object_id_value_(object_id_value),
                             function_(function),
                             when_(when) {
    }

    uint64_t get_object_id_value() const {
      return object_id_value_;
    }

    time_point get_when() const {
      return when_;
    }

    void call_function() const {
      function_();
    }

  private:
    uint64_t object_id_value_;
    std::function<void()> function_;
    time_point when_;
  };

  std::weak_ptr<time_source> weak_time_source_;
  mutable std::mutex weak_time_source_mutex_;

  std::thread worker_thread_;
  std::thread::id worker_thread_id_;
  std::shared_ptr<thread_wait> worker_thread_id_wait_;

  std::deque<std::shared_ptr<entry>> queue_;
  bool exit_ = false;

  // Protects queue_, exit_, and the worker thread wait condition.
  // Lock order: acquire object_ids_mutex_ before mutex_ if both are needed.
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
} // namespace pqrs::dispatcher
