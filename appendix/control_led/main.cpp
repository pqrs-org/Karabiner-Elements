#include "boost_defs.hpp"

#include "human_interface_device.hpp"
#include "iokit_utility.hpp"
#include "logger.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <boost/bind.hpp>
#include <iostream>
#include <mach/mach_time.h>

namespace {
class control_led final {
public:
  control_led(const control_led&) = delete;

  control_led(void) {
    manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager_) {
      return;
    }

    auto device_matching_dictionaries = krbn::iokit_utility::create_device_matching_dictionaries({
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_keyboard),
    });
    if (device_matching_dictionaries) {
      IOHIDManagerSetDeviceMatchingMultiple(manager_, device_matching_dictionaries);
      CFRelease(device_matching_dictionaries);

      IOHIDManagerRegisterDeviceMatchingCallback(manager_, static_device_matching_callback, this);
      IOHIDManagerRegisterDeviceRemovalCallback(manager_, static_device_removal_callback, this);

      IOHIDManagerScheduleWithRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    }
  }

private:
  static void static_device_matching_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess || !device) {
      return;
    }

    auto self = static_cast<control_led*>(context);
    if (!self) {
      return;
    }

    self->device_matching_callback(device);
  }

  void device_matching_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    krbn::iokit_utility::log_matching_device(device);

    hids_[device] = std::make_unique<krbn::human_interface_device>(device);
    auto& dev = hids_[device];

    auto kr = dev->open();
    if (kr != kIOReturnSuccess) {
      krbn::logger::get_logger().error("failed to dev->open(). {0}", kr);
    }
    dev->schedule();

    if (auto caps_lock_led_state = dev->get_caps_lock_led_state()) {
      switch (*caps_lock_led_state) {
        case krbn::led_state::on:
          krbn::logger::get_logger().info("caps_lock_led_state is on.");
          break;
        case krbn::led_state::off:
          krbn::logger::get_logger().info("caps_lock_led_state is off.");
          break;
      }

      if (caps_lock_led_state == krbn::led_state::on) {
        dev->set_caps_lock_led_state(krbn::led_state::off);
      } else {
        dev->set_caps_lock_led_state(krbn::led_state::on);
      }

      krbn::logger::get_logger().info("set_caps_lock_led_state is called.");

    } else {
      krbn::logger::get_logger().info("failed to get caps_lock_led_state.");
    }
  }

  static void static_device_removal_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess || !device) {
      return;
    }

    auto self = static_cast<control_led*>(context);
    if (!self) {
      return;
    }

    self->device_removal_callback(device);
  }

  void device_removal_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    krbn::iokit_utility::log_removal_device(device);

    auto it = hids_.find(device);
    if (it != hids_.end()) {
      auto& dev = it->second;
      if (dev) {
        hids_.erase(it);
      }
    }
  }

  IOHIDManagerRef _Nullable manager_;
  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<krbn::human_interface_device>> hids_;
};
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  if (getuid() != 0) {
    krbn::logger::get_logger().error("control_led requires root privilege.");
  }

  control_led d;
  CFRunLoopRun();
  return 0;
}
