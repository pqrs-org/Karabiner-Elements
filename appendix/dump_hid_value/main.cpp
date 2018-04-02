#include "boost_defs.hpp"

#include "human_interface_device.hpp"
#include "human_interface_device_observer.hpp"
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
#include <boost/optional/optional_io.hpp>
#include <iostream>
#include <mach/mach_time.h>

namespace {
class dump_hid_value final {
public:
  dump_hid_value(const dump_hid_value&) = delete;

  dump_hid_value(void) {
    manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager_) {
      return;
    }

    auto device_matching_dictionaries = krbn::iokit_utility::create_device_matching_dictionaries({
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_keyboard),
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_mouse),
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_pointer),
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

    auto self = static_cast<dump_hid_value*>(context);
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

    auto observer = std::make_shared<krbn::human_interface_device_observer>(device);
    observer->hid_value_arrived.connect([&](auto& human_interface_device_observer,
                                            auto& hid_value) {
      print_hid_value(human_interface_device_observer, hid_value);
    });
    hid_observers_[device] = observer;
  }

  static void static_device_removal_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess || !device) {
      return;
    }

    auto self = static_cast<dump_hid_value*>(context);
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

    auto it = hid_observers_.find(device);
    if (it != hid_observers_.end()) {
      hid_observers_.erase(it);
    }
  }

  void print_hid_value(const krbn::human_interface_device_observer& human_interface_device_observer,
                       const krbn::hid_value& hid_value) const {
    if (auto key_code = krbn::types::make_key_code(hid_value)) {
      if (auto key_code_name = krbn::types::make_key_code_name(*key_code)) {
        std::cout << *key_code_name;
      } else {
        std::cout << "key_code:" << *key_code;
      }

      if (hid_value.get_integer_value()) {
        std::cout << " key_down";
      } else {
        std::cout << " key_up";
      }

      std::cout << std::endl;
      std::cout << "  is_grabbable:" << human_interface_device_observer.is_grabbable(false) << std::endl;
      return;
    }

    if (auto consumer_key_code = krbn::types::make_consumer_key_code(hid_value)) {
      if (auto consumer_key_code_name = krbn::types::make_consumer_key_code_name(*consumer_key_code)) {
        std::cout << *consumer_key_code_name;
      } else {
        std::cout << "consumer_key_code:" << *consumer_key_code;
      }

      if (hid_value.get_integer_value()) {
        std::cout << " key_down";
      } else {
        std::cout << " key_up";
      }

      std::cout << std::endl;
      std::cout << "  is_grabbable:" << human_interface_device_observer.is_grabbable(false) << std::endl;
      return;
    }

    if (auto pointing_button = krbn::types::make_pointing_button(hid_value)) {
      if (auto pointing_button_name = krbn::types::make_pointing_button_name(*pointing_button)) {
        std::cout << *pointing_button_name;
      } else {
        std::cout << "pointing_button:" << *pointing_button;
      }

      if (hid_value.get_integer_value()) {
        std::cout << " key_down";
      } else {
        std::cout << " key_up";
      }

      std::cout << std::endl;
      std::cout << "  is_grabbable:" << human_interface_device_observer.is_grabbable(false) << std::endl;
      return;
    }

    if (hid_value.conforms_to(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_x)) {
      std::cout << "pointing_motion x:" << hid_value.get_integer_value() << std::endl;
      return;
    }
    if (hid_value.conforms_to(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_y)) {
      std::cout << "pointing_motion y:" << hid_value.get_integer_value() << std::endl;
      return;
    }
    if (hid_value.conforms_to(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_wheel)) {
      std::cout << "pointing_motion vertical_wheel:" << hid_value.get_integer_value() << std::endl;
      return;
    }
    if (hid_value.conforms_to(krbn::hid_usage_page::consumer, krbn::hid_usage::csmr_acpan)) {
      std::cout << "pointing_motion horizontal_wheel:" << hid_value.get_integer_value() << std::endl;
      return;
    }

    if (hid_value.conforms_to(krbn::hid_usage_page::keyboard_or_keypad, krbn::hid_usage(0x1)) ||
        hid_value.conforms_to(krbn::hid_usage_page::keyboard_or_keypad, krbn::hid_usage(0x2)) ||
        hid_value.conforms_to(krbn::hid_usage_page::keyboard_or_keypad, krbn::hid_usage(0x3)) ||
        hid_value.conforms_to(krbn::hid_usage_page::keyboard_or_keypad, krbn::hid_usage(0xffffffff)) ||
        hid_value.conforms_to(krbn::hid_usage_page::consumer, krbn::hid_usage(0xffffffff)) ||
        hid_value.conforms_to(krbn::hid_usage_page::apple_vendor_keyboard, krbn::hid_usage(0xffffffff)) ||
        hid_value.conforms_to(krbn::hid_usage_page::apple_vendor_top_case, krbn::hid_usage(0xffffffff))) {
      return;
    }

    std::cout << "hid_value:" << hid_value.to_json() << std::endl;
  }

  IOHIDManagerRef _Nullable manager_;
  std::unordered_map<IOHIDDeviceRef, std::shared_ptr<krbn::human_interface_device_observer>> hid_observers_;
};
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  dump_hid_value d;
  CFRunLoopRun();
  return 0;
}
