#pragma once

// pqrs::cf::run_loop_thread v2.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::cf::run_loop_thread` can be used safely in a multi-threaded environment.

#include <pqrs/cf/cf_ptr.hpp>
#include <pqrs/thread_wait.hpp>
#include <thread>

namespace pqrs {
namespace cf {
class run_loop_thread final {
public:
  run_loop_thread(const run_loop_thread&) = delete;

  run_loop_thread(void) {
    running_wait_ = make_thread_wait();

    thread_ = std::thread([this] {
      run_loop_ = CFRunLoopGetCurrent();

      // Append empty source to prevent immediately quitting of `CFRunLoopRun`.

      auto context = CFRunLoopSourceContext();
      context.perform = perform;
      auto source = CFRunLoopSourceCreate(kCFAllocatorDefault,
                                          0,
                                          &context);

      CFRunLoopAddSource(*run_loop_,
                         source,
                         kCFRunLoopCommonModes);

      // Run

      CFRunLoopPerformBlock(*run_loop_,
                            kCFRunLoopCommonModes,
                            ^{
                              running_wait_->notify();
                            });

      CFRunLoopRun();

      // Remove source

      CFRunLoopRemoveSource(*run_loop_,
                            source,
                            kCFRunLoopCommonModes);

      CFRelease(source);
    });
  }

  ~run_loop_thread(void) {
    if (thread_.joinable()) {
      // We have to call `terminate` before destroy run_loop_thread.
      abort();
    }

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
    // We wait until running to avoid a segmentation fault which is described in `enqueue`.

    wait_until_running();

    return *run_loop_;
  }

  void wake(void) const {
    // Do not touch run_loop_ until `CFRunLoopRun` is called.
    // A segmentation fault occurs if we touch `run_loop_` while `CFRunLoopRun' is processing.

    wait_until_running();

    CFRunLoopWakeUp(*run_loop_);
  }

  void enqueue(void (^_Nonnull block)(void)) const {
    // Do not call `CFRunLoopPerformBlock` until `CFRunLoopRun` is called.
    // A segmentation fault occurs if we call `CFRunLoopPerformBlock` while `CFRunLoopRun' is processing.

    wait_until_running();

    CFRunLoopPerformBlock(*run_loop_,
                          kCFRunLoopCommonModes,
                          block);

    CFRunLoopWakeUp(*run_loop_);
  }

private:
  void wait_until_running(void) const {
    running_wait_->wait_notice();
  }

  static void perform(void* _Nullable info) {
  }

  std::thread thread_;
  cf_ptr<CFRunLoopRef> run_loop_;
  std::shared_ptr<thread_wait> running_wait_;
};
} // namespace cf
} // namespace pqrs
