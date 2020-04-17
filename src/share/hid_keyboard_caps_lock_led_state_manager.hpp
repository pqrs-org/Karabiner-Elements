#pragma once

#include "types.hpp"
#include <mach/mach_time.h>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_hid_device.hpp>

namespace krbn {
class hid_keyboard_caps_lock_led_state_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  hid_keyboard_caps_lock_led_state_manager(IOHIDDeviceRef device) : dispatcher_client(),
                                                                    device_(device),
                                                                    timer_(*this),
                                                                    started_(false) {
    if (device_) {
      pqrs::osx::iokit_hid_device hid_device(*device_);
      for (const auto& e : hid_device.make_elements()) {
        auto usage_page = pqrs::hid::usage_page::value_t(IOHIDElementGetUsagePage(*e));
        auto usage = pqrs::hid::usage::value_t(IOHIDElementGetUsage(*e));

        if (usage_page == pqrs::hid::usage_page::leds &&
            usage == pqrs::hid::usage::led::caps_lock) {
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
                                                          *element_,
                                                          mach_absolute_time(),
                                                          *integer_value)) {
          IOHIDDeviceSetValue(*device_, *element_, value);

          CFRelease(value);
        }
      }
    }
  }

  std::optional<CFIndex> make_integer_value(void) const {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (state_ && element_) {
      if (*state_ == led_state::on) {
        return IOHIDElementGetLogicalMax(*element_);
      } else {
        return IOHIDElementGetLogicalMin(*element_);
      }
    }

    return std::nullopt;
  }

  pqrs::cf::cf_ptr<IOHIDDeviceRef> device_;
  pqrs::cf::cf_ptr<IOHIDElementRef> element_;
  std::optional<led_state> state_;
  mutable std::mutex state_mutex_;
  pqrs::dispatcher::extra::timer timer_;
  bool started_;
};
} // namespace krbn
