#pragma once

// pqrs::iokit_hid_queue_value_monitor v1.2

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <chrono>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/cf_run_loop_thread.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_hid_device.hpp>
#include <pqrs/osx/iokit_return.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace pqrs {
namespace osx {
class iokit_hid_queue_value_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> started;
  nod::signal<void(void)> stopped;
  nod::signal<void(std::shared_ptr<std::vector<cf_ptr<IOHIDValueRef>>>)> values_arrived;
  nod::signal<void(const std::string&, iokit_return)> error_occurred;

  // Methods

  iokit_hid_queue_value_monitor(const iokit_hid_queue_value_monitor&) = delete;

  iokit_hid_queue_value_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
                                IOHIDDeviceRef device) : hid_device_(device),
                                                         open_timer_(*this),
                                                         last_open_error_(kIOReturnSuccess) {
    cf_run_loop_thread_ = std::make_unique<cf_run_loop_thread>();

    enqueue_to_dispatcher([this] {
      if (hid_device_.get_device()) {
        IOHIDDeviceRegisterRemovalCallback(*(hid_device_.get_device()),
                                           static_device_removal_callback,
                                           this);

        IOHIDDeviceScheduleWithRunLoop(*(hid_device_.get_device()),
                                       cf_run_loop_thread_->get_run_loop(),
                                       kCFRunLoopCommonModes);
      }
    });
  }

  virtual ~iokit_hid_queue_value_monitor(void) {
    detach_from_dispatcher([this] {
      stop();

      if (hid_device_.get_device()) {
        IOHIDDeviceUnscheduleFromRunLoop(*(hid_device_.get_device()),
                                         cf_run_loop_thread_->get_run_loop(),
                                         kCFRunLoopCommonModes);
      }
    });

    cf_run_loop_thread_->terminate();
    cf_run_loop_thread_ = nullptr;
  }

  void async_start(IOOptionBits open_options,
                   std::chrono::milliseconds open_timer_interval) {
    enqueue_to_dispatcher([this, open_options, open_timer_interval] {
      start(open_options, open_timer_interval);
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

private:
  bool start(IOOptionBits open_options,
             std::chrono::milliseconds open_timer_interval) {
    open_timer_.start(
        [this, open_options] {
          if (hid_device_.get_device()) {
            if (!open_options_) {
              iokit_return r = IOHIDDeviceOpen(*(hid_device_.get_device()),
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

              open_options_ = open_options;

              start_queue();

              enqueue_to_dispatcher([this] {
                started();
              });
            }
          }

          open_timer_.stop();
        },
        open_timer_interval);
  }

  void stop(void) {
    if (hid_device_.get_device()) {
      stop_queue();

      if (open_options_) {
        IOHIDDeviceClose(*(hid_device_.get_device()),
                         *open_options_);

        open_options_ = std::nullopt;

        enqueue_to_dispatcher([this] {
          stopped();
        });
      }
    }

    open_timer_.stop();
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
                                      cf_run_loop_thread_->get_run_loop(),
                                      kCFRunLoopCommonModes);

        IOHIDQueueStart(*queue_);
      }
    }
  }

  void stop_queue(void) {
    if (queue_) {
      IOHIDQueueStop(*queue_);

      IOHIDQueueUnscheduleFromRunLoop(*queue_,
                                      cf_run_loop_thread_->get_run_loop(),
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
    stop();
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
    enqueue_to_dispatcher([this] {
      if (queue_) {
        auto values = std::make_shared<std::vector<cf_ptr<IOHIDValueRef>>>();

        while (auto v = IOHIDQueueCopyNextValueWithTimeout(*queue_, 0.0)) {
          values->emplace_back(v);

          CFRelease(v);
        }

        enqueue_to_dispatcher([this, values] {
          values_arrived(values);
        });
      }
    });
  }

  iokit_hid_device hid_device_;
  std::unique_ptr<cf_run_loop_thread> cf_run_loop_thread_;
  pqrs::dispatcher::extra::timer open_timer_;
  std::optional<IOOptionBits> open_options_;
  iokit_return last_open_error_;
  cf_ptr<IOHIDQueueRef> queue_;
};
} // namespace osx
} // namespace pqrs
