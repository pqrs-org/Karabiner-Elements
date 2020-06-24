#pragma once

#include "iokit_utility.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <mach/mach_time.h>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_hid_device.hpp>
#include <pqrs/osx/iokit_hid_element.hpp>
#include <pqrs/osx/iokit_return.hpp>

namespace krbn {
class hid_keyboard_caps_lock_led_state_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  hid_keyboard_caps_lock_led_state_manager(IOHIDDeviceRef device) : dispatcher_client(),
                                                                    device_(device),
                                                                    timer_(*this),
                                                                    started_(false) {
    if (device_) {
      pqrs::osx::iokit_hid_device hid_device(*device_);
      for (const auto& element : hid_device.make_elements()) {
        pqrs::osx::iokit_hid_element e(*element);

        if (e.get_usage_page() == pqrs::hid::usage_page::leds &&
            e.get_usage() == pqrs::hid::usage::led::caps_lock &&
            e.get_type() == pqrs::osx::iokit_hid_element_type::output) {
          logger::get_logger()->info(
              "caps lock is found on {0}",
              iokit_utility::make_device_name(*device_));

          element_ = e;
        }
      }
    }
  }

  ~hid_keyboard_caps_lock_led_state_manager(void) {
    detach_from_dispatcher([this] {
      timer_.stop();
    });
  }

  void set_state(std::optional<led_state> value) {
    std::lock_guard<std::mutex> lock(state_mutex_);

    // Skip if new state is same as the last state in order to avoid a possibility of infinite calling of set_state.
    if (state_ == value) {
      return;
    }

    state_ = value;

    enqueue_to_dispatcher([this] {
      update_caps_lock_led();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      started_ = true;

      timer_.start(
          [this] {
            update_caps_lock_led();
          },
          std::chrono::milliseconds(3000));
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      started_ = false;

      timer_.stop();
    });
  }

private:
  void update_caps_lock_led(void) const {
    if (!started_) {
      return;
    }

    // macOS 10.12 sometimes synchronize caps lock LED to internal keyboard caps lock state.
    // The behavior causes LED state mismatch because
    // the caps lock state of karabiner_grabber is independent from the hardware caps lock state.
    // Thus, we monitor the LED state and update it if needed.

    if (auto integer_value = make_integer_value()) {
      if (device_ && element_) {
        if (auto value = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault,
                                                          element_.get_raw_ptr(),
                                                          mach_absolute_time(),
                                                          *integer_value)) {
          // We have to use asynchronous method in order to prevent deadlock.
          IOHIDDeviceSetValueWithCallback(
              *device_,
              element_.get_raw_ptr(),
              value,
              0.1,
              [](void* context, IOReturn result, void* sender, IOHIDValueRef value) {
                pqrs::osx::iokit_return r(result);
                if (!r) {
                  logger::get_logger()->warn("update_caps_lock_led is failed: {0}", r);
                }
              },
              nullptr);

          CFRelease(value);
        }
      }
    }
  }

  std::optional<CFIndex> make_integer_value(void) const {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (state_ && element_) {
      if (*state_ == led_state::on) {
        return element_.get_logical_max();
      } else {
        return element_.get_logical_min();
      }
    }

    return std::nullopt;
  }

  pqrs::cf::cf_ptr<IOHIDDeviceRef> device_;
  pqrs::osx::iokit_hid_element element_;
  std::optional<led_state> state_;
  mutable std::mutex state_mutex_;
  pqrs::dispatcher::extra::timer timer_;
  bool started_;
};
} // namespace krbn
