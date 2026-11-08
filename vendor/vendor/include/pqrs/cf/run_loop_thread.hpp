#pragma once

// pqrs::cf::run_loop_thread v4.0.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::cf::run_loop_thread` can be used safely in a multi-threaded environment.

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <pqrs/cf/cf_ptr.hpp>
#include <pqrs/gsl.hpp>
#include <pqrs/thread_wait.hpp>
#include <thread>
#include <utility>

namespace pqrs::cf {
class run_loop_thread final {
public:
  // `execution_wrapper` must invoke the supplied function exactly once,
  // synchronously, and return after the function has finished.
  // The supplied function runs one CFRunLoopRunInMode activation and may block
  // for up to `run_loop_run_timeout`, even when the run loop handles no events.
  using execution_wrapper = std::function<void(const std::function<void()>&)>;

  enum class failure_policy {
    abort,
    exit,
  };

  class extra {
  public:
#include "run_loop_thread/extra/shared_run_loop_thread.hpp"
  };

  run_loop_thread(const run_loop_thread&) = delete;
  run_loop_thread& operator=(const run_loop_thread&) = delete;

  run_loop_thread(failure_policy policy = failure_policy::abort)
      : run_loop_thread(policy, {}) {
  }

  run_loop_thread(failure_policy policy,
                  execution_wrapper execution_wrapper)
      : failure_policy_(policy),
        execution_wrapper_(std::move(execution_wrapper)) {
    using namespace std::chrono_literals;

    thread_ = std::thread([this] {
      //
      // Preparation
      //

      // Add a keep-alive source to prevent `CFRunLoopRunInMode` from immediately quitting.

      auto context = CFRunLoopSourceContext();
      context.info = reinterpret_cast<void*>(this);
      context.perform = [](void* _Nullable info) {
        auto self = reinterpret_cast<run_loop_thread*>(info);
        self->ready_.store(true);
      };

      {
        std::lock_guard<std::mutex> lock(keep_alive_source_mutex_);

        run_loop_ = CFRunLoopGetCurrent();

        keep_alive_source_ = adopt_cf_ptr(CFRunLoopSourceCreate(kCFAllocatorDefault,
                                                                0,
                                                                &context));

        CFRunLoopAddSource(*run_loop_,
                           *keep_alive_source_,
                           kCFRunLoopCommonModes);
      }

      //
      // Run
      //

      // Run the run loop in finite activations so that `execution_wrapper_`
      // has a well-defined lifetime outside CFRunLoop's internal scopes.
      // Managing that lifetime with run-loop observer callbacks is unsafe:
      // the callbacks run among CFRunLoop's internal autorelease-pool changes,
      // so pushing and popping a pool there can violate LIFO ordering.
      // The finite timeout also recreates the wrapper while the run loop is idle.
      // `returnAfterSourceHandled` is false because source handling and
      // CFRunLoopPerformBlock execution are not ordered; returning after one
      // source could leave an enqueued block for a later activation.
      constexpr CFTimeInterval run_loop_run_timeout = 1.0;

      while (true) {
        auto result = kCFRunLoopRunFinished;

        const auto run = [&] {
          result = CFRunLoopRunInMode(kCFRunLoopDefaultMode,
                                      run_loop_run_timeout,
                                      false);
        };

        if (execution_wrapper_) {
          execution_wrapper_(run);
        } else {
          run();
        }

        if (result == kCFRunLoopRunFinished ||
            result == kCFRunLoopRunStopped) {
          break;
        }
      }

      //
      // Remove source
      //

      CFRunLoopRemoveSource(*run_loop_,
                            *keep_alive_source_,
                            kCFRunLoopCommonModes);
    });

    // Wait until CFRunLoop is running.

    auto since = std::chrono::system_clock::now();
    while (!ready_) {
      auto now = std::chrono::system_clock::now();
      if (now - since > 3s) {
        // Although this does not usually happen, it is reached when CFRunLoop processing does not start due to a problem with CFRunLoop.
        // Abort because it is irrecoverable.
        if (failure_policy_ == failure_policy::exit) {
          // `std::quick_exit` causes a "Symbol not found: _quick_exit" error on macOS 13,
          // so we use `std::_Exit` instead.
          std::_Exit(EXIT_FAILURE);
        } else {
          abort();
        }
      }

      {
        std::lock_guard<std::mutex> lock(keep_alive_source_mutex_);

        if (run_loop_ && keep_alive_source_) {
          CFRunLoopSourceSignal(*keep_alive_source_);
          CFRunLoopWakeUp(*run_loop_);
        }
      }

      // The period of time should be as short as possible, as the thread sleeps at least once here.
      std::this_thread::sleep_for(10ms);
    }
  }

  ~run_loop_thread() noexcept {
    if (thread_.joinable()) {
      // We have to call `terminate` before destroy run_loop_thread.
      abort();
    }

    keep_alive_source_ = nullptr;
    run_loop_ = nullptr;
  }

  void terminate() {
    std::lock_guard<std::mutex> lock(thread_mutex_);

    if (thread_.joinable()) {
      enqueue(^{
        CFRunLoopStop(*run_loop_);
      });

      thread_.join();
    }
  }

  [[nodiscard]] CFRunLoopRef _Nonnull get_run_loop() const noexcept {
    return *run_loop_;
  }

  void wake() const noexcept {
    CFRunLoopWakeUp(*run_loop_);
  }

  void add_source(CFRunLoopSourceRef _Nullable source,
                  CFRunLoopMode _Nonnull mode = kCFRunLoopCommonModes) noexcept {
    if (source) {
      CFRunLoopAddSource(*run_loop_,
                         source,
                         mode);

      CFRunLoopWakeUp(*run_loop_);
    }
  }

  void remove_source(CFRunLoopSourceRef _Nullable source,
                     CFRunLoopMode _Nonnull mode = kCFRunLoopCommonModes) noexcept {
    if (source) {
      CFRunLoopRemoveSource(*run_loop_,
                            source,
                            mode);

      CFRunLoopWakeUp(*run_loop_);
    }
  }

  void enqueue(void (^_Nonnull block)()) const noexcept {
    CFRunLoopPerformBlock(*run_loop_,
                          kCFRunLoopCommonModes,
                          block);

    CFRunLoopWakeUp(*run_loop_);
  }

private:
  std::thread thread_;
  std::mutex thread_mutex_;
  cf_ptr<CFRunLoopRef> run_loop_;

  cf_ptr<CFRunLoopSourceRef> keep_alive_source_;
  std::mutex keep_alive_source_mutex_;
  std::atomic<bool> ready_{false};
  failure_policy failure_policy_;
  execution_wrapper execution_wrapper_;
};
} // namespace pqrs::cf
