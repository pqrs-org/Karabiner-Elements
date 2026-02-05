#pragma once

// pqrs::cf::run_loop_thread v2.8

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::cf::run_loop_thread` can be used safely in a multi-threaded environment.

#include <cstdlib>
#include <pqrs/cf/cf_ptr.hpp>
#include <pqrs/thread_wait.hpp>
#include <thread>

namespace pqrs {
namespace cf {
class run_loop_thread final {
public:
  enum class failure_policy {
    abort,
    exit,
  };

  class extra {
  public:
#include "run_loop_thread/extra/shared_run_loop_thread.hpp"
  };

  run_loop_thread(const run_loop_thread&) = delete;

  run_loop_thread(failure_policy policy = failure_policy::abort)
      : failure_policy_(policy) {
    std::atomic<bool> ready = false;

    thread_ = std::thread([this, &ready] {
      run_loop_ = CFRunLoopGetCurrent();

      // Append a source to prevent immediately quitting of `CFRunLoopRun`.

      auto context = CFRunLoopSourceContext();
      context.info = reinterpret_cast<void*>(&ready);
      context.perform = [](void* _Nullable info) {
        auto ready = reinterpret_cast<std::atomic<bool>*>(info);
        *ready = true;
      };

      {
        std::lock_guard<std::mutex> lock(initial_source_mutex_);

        initial_source_ = CFRunLoopSourceCreate(kCFAllocatorDefault,
                                                0,
                                                &context);
        CFRelease(*initial_source_);

        CFRunLoopAddSource(*run_loop_,
                           *initial_source_,
                           kCFRunLoopCommonModes);
      }

      // Run

      CFRunLoopRun();

      // Remove source

      CFRunLoopRemoveSource(*run_loop_,
                            *initial_source_,
                            kCFRunLoopCommonModes);
    });

    // Wait until CFRunLoop is running.

    auto since = std::chrono::system_clock::now();
    while (!ready) {
      auto now = std::chrono::system_clock::now();
      if (now - since > std::chrono::milliseconds(3000)) {
        // Although this does not usually happen, it is reached when CFRunLoop processing does not start due to a problem with CFRunLoop.
        // Abort because it is irrecoverable.
        if (failure_policy_ == failure_policy::exit) {
          std::exit(EXIT_FAILURE);
        } else {
          abort();
        }
      }

      {
        std::lock_guard<std::mutex> lock(initial_source_mutex_);

        if (run_loop_ && initial_source_) {
          CFRunLoopSourceSignal(*initial_source_);
          CFRunLoopWakeUp(*run_loop_);
        }
      }

      // The period of time should be as short as possible, as the thread sleeps at least once here.
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  ~run_loop_thread(void) {
    if (thread_.joinable()) {
      // We have to call `terminate` before destroy run_loop_thread.
      abort();
    }

    initial_source_ = nullptr;
    run_loop_ = nullptr;
  }

  void terminate(void) {
    enqueue(^{
      CFRunLoopStop(*run_loop_);
    });

    if (thread_.joinable()) {
      thread_.join();
    }
  }

  CFRunLoopRef _Nonnull get_run_loop(void) const {
    return *run_loop_;
  }

  void wake(void) const {
    CFRunLoopWakeUp(*run_loop_);
  }

  void add_source(CFRunLoopSourceRef _Nullable source,
                  CFRunLoopMode _Nonnull mode = kCFRunLoopCommonModes) {
    if (source) {
      CFRunLoopAddSource(*run_loop_,
                         source,
                         mode);

      CFRunLoopWakeUp(*run_loop_);
    }
  }

  void remove_source(CFRunLoopSourceRef _Nullable source,
                     CFRunLoopMode _Nonnull mode = kCFRunLoopCommonModes) {
    if (source) {
      CFRunLoopRemoveSource(*run_loop_,
                            source,
                            mode);

      CFRunLoopWakeUp(*run_loop_);
    }
  }

  void enqueue(void (^_Nonnull block)(void)) const {
    CFRunLoopPerformBlock(*run_loop_,
                          kCFRunLoopCommonModes,
                          block);

    CFRunLoopWakeUp(*run_loop_);
  }

private:
  std::thread thread_;
  cf_ptr<CFRunLoopRef> run_loop_;

  cf_ptr<CFRunLoopSourceRef> initial_source_;
  std::mutex initial_source_mutex_;
  failure_policy failure_policy_;
};
} // namespace cf
} // namespace pqrs
