#pragma once

// pqrs::osx::iokit_hid_queue_value_monitor v2.2

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <chrono>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/cf/run_loop_thread.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_hid_device.hpp>
#include <pqrs/osx/iokit_return.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace pqrs {
namespace osx {
class iokit_hid_queue_value_monitor final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(void)> started;
  nod::signal<void(void)> stopped;
  nod::signal<void(std::shared_ptr<std::vector<cf::cf_ptr<IOHIDValueRef>>>)> values_arrived;
  nod::signal<void(const std::string&, iokit_return)> error_occurred;

  // Methods

  iokit_hid_queue_value_monitor(const iokit_hid_queue_value_monitor&) = delete;

  // CFRunLoopRun may get stuck in rare cases if cf::run_loop_thread generation is repeated frequently in macOS 13.
  // If such a condition occurs, cf::run_loop_thread detects it and calls abort to avoid it.
  // However, to avoid the problem itself, cf::run_loop_thread should be provided externally instead of having it internally.
  iokit_hid_queue_value_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
                                std::shared_ptr<cf::run_loop_thread> run_loop_thread,
                                IOHIDDeviceRef device)
      : dispatcher_client(weak_dispatcher),
        run_loop_thread_(run_loop_thread),
        hid_device_(device),
        open_timer_(*this),
        last_open_error_(kIOReturnSuccess) {
    // Schedule device

    auto wait = make_thread_wait();

    run_loop_thread_->enqueue(^{
      if (auto d = hid_device_.get_device()) {
        IOHIDDeviceRegisterRemovalCallback(*d,
                                           static_device_removal_callback,
                                           this);

        IOHIDDeviceScheduleWithRunLoop(*d,
                                       run_loop_thread_->get_run_loop(),
                                       kCFRunLoopCommonModes);
      }

      wait->notify();
    });

    wait->wait_notice();
  }

  virtual ~iokit_hid_queue_value_monitor(void) {
    //
    // dispatcher_client
    //

    detach_from_dispatcher();

    //
    // run_loop_thread
    //

    auto wait = make_thread_wait();

    run_loop_thread_->enqueue(^{
      stop({.check_requested_open_options = false});

      if (auto d = hid_device_.get_device()) {
        IOHIDDeviceUnscheduleFromRunLoop(*d,
                                         run_loop_thread_->get_run_loop(),
                                         kCFRunLoopCommonModes);
      }

      wait->notify();
    });

    wait->wait_notice();
  }

  void async_start(IOOptionBits open_options,
                   std::chrono::milliseconds open_timer_interval) {
    {
      std::lock_guard<std::mutex> lock(open_options_mutex_);

      requested_open_options_ = open_options;
    }

    run_loop_thread_->enqueue(^{
      open_timer_.start(
          [this] {
            run_loop_thread_->enqueue(^{
              start();
            });
          },
          open_timer_interval);
    });
  }

  void async_stop(void) {
    {
      std::lock_guard<std::mutex> lock(open_options_mutex_);

      requested_open_options_ = std::nullopt;
    }

    run_loop_thread_->enqueue(^{
      stop({.check_requested_open_options = true});
    });
  }

  bool seized() const {
    std::lock_guard<std::mutex> lock(open_options_mutex_);

    return current_open_options_ != std::nullopt
               ? (*current_open_options_ & kIOHIDOptionsTypeSeizeDevice)
               : false;
  }

private:
  void start(void) {
    bool needs_stop = false;
    IOOptionBits open_options = kIOHIDOptionsTypeNone;

    auto device = hid_device_.get_device();
    if (!device) {
      goto finish;
    }

    //
    // Check requested_open_options_
    //

    {
      std::lock_guard<std::mutex> lock(open_options_mutex_);

      if (requested_open_options_ == std::nullopt ||
          requested_open_options_ == current_open_options_) {
        goto finish;
      }

      if (current_open_options_) {
        needs_stop = true;
      }

      open_options = *requested_open_options_;
    }

    if (needs_stop) {
      stop({.check_requested_open_options = false});
    }

    //
    // Open the device
    //

    // Start queue before `IOHIDDeviceOpen` in order to avoid events drop.
    start_queue();

    {
      iokit_return r = IOHIDDeviceOpen(*device,
                                       open_options);
      if (!r) {
        if (last_open_error_ != r) {
          last_open_error_ = r;
          enqueue_to_dispatcher([this, r] {
            error_occurred("IOHIDDeviceOpen is failed.", r);
          });
        }

        // Retry
        return;
      }
    }

    {
      std::lock_guard<std::mutex> lock(open_options_mutex_);

      current_open_options_ = requested_open_options_;
    }

    enqueue_to_dispatcher([this] {
      started();
    });

  finish:
    open_timer_.stop();
  }

  struct stop_arguments {
    bool check_requested_open_options;
  };
  void stop(stop_arguments args) {
    // Since `stop()` can be called from within `start()`,
    // we must not stop `open_timer_` in `stop()` in order to preserve the retry when `IOHIDDeviceOpen` error.

    IOOptionBits open_options = kIOHIDOptionsTypeNone;

    auto device = hid_device_.get_device();
    if (!device) {
      return;
    }

    {
      std::lock_guard<std::mutex> lock(open_options_mutex_);

      if (current_open_options_ == std::nullopt) {
        return;
      }

      if (args.check_requested_open_options &&
          requested_open_options_ != std::nullopt) {
        return;
      }

      open_options = *current_open_options_;
    }

    stop_queue();

    IOHIDDeviceClose(*device,
                     open_options);

    {
      std::lock_guard<std::mutex> lock(open_options_mutex_);

      current_open_options_ = std::nullopt;
    }

    enqueue_to_dispatcher([this] {
      stopped();
    });
  }

  void start_queue(void) {
    if (!queue_) {
      const CFIndex depth = 1024;
      queue_ = hid_device_.make_queue(depth);

      if (queue_) {
        for (const auto& e : hid_device_.make_elements()) {
          IOHIDQueueAddElement(*queue_, *e);
        }

        IOHIDQueueRegisterValueAvailableCallback(*queue_,
                                                 static_queue_value_available_callback,
                                                 this);

        IOHIDQueueScheduleWithRunLoop(*queue_,
                                      run_loop_thread_->get_run_loop(),
                                      kCFRunLoopCommonModes);

        IOHIDQueueStart(*queue_);
      }
    }
  }

  void stop_queue(void) {
    if (queue_) {
      IOHIDQueueStop(*queue_);

      // IOHIDQueueUnscheduleFromRunLoop might cause SIGSEGV if it is not called in run_loop_thread_.

      IOHIDQueueUnscheduleFromRunLoop(*queue_,
                                      run_loop_thread_->get_run_loop(),
                                      kCFRunLoopCommonModes);

      queue_ = nullptr;
    }
  }

  static void static_device_removal_callback(void* context,
                                             IOReturn result,
                                             void* sender) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<iokit_hid_queue_value_monitor*>(context);
    if (!self) {
      return;
    }

    self->device_removal_callback();
  }

  void device_removal_callback(void) {
    stop({.check_requested_open_options = false});
  }

  static void static_queue_value_available_callback(void* context,
                                                    IOReturn result,
                                                    void* sender) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<iokit_hid_queue_value_monitor*>(context);
    if (!self) {
      return;
    }

    self->queue_value_available_callback();
  }

  void queue_value_available_callback(void) {
    if (queue_) {
      auto values = std::make_shared<std::vector<cf::cf_ptr<IOHIDValueRef>>>();

      while (auto v = IOHIDQueueCopyNextValueWithTimeout(*queue_, 0.0)) {
        values->emplace_back(v);

        CFRelease(v);
      }

      // macOS Catalina (10.15) call the `ValueAvailableCallback`
      // even if `IOHIDDeviceOpen` is failed. (A bug of macOS)
      // Thus, we should ignore the events when `IOHIDDeviceOpen` is failed.
      // (== open_options_ == std::nullopt)

      {
        std::lock_guard<std::mutex> lock(open_options_mutex_);

        if (!current_open_options_) {
          return;
        }
      }

      enqueue_to_dispatcher([this, values] {
        values_arrived(values);
      });
    }
  }

  std::shared_ptr<cf::run_loop_thread> run_loop_thread_;

  iokit_hid_device hid_device_;
  dispatcher::extra::timer open_timer_;
  std::optional<IOOptionBits> requested_open_options_;
  std::optional<IOOptionBits> current_open_options_;
  mutable std::mutex open_options_mutex_;
  iokit_return last_open_error_;
  cf::cf_ptr<IOHIDQueueRef> queue_;
};
} // namespace osx
} // namespace pqrs
