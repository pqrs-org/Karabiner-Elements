#include "boost_defs.hpp"

#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtual_hid_device.hpp"
#include "iokit_utility.hpp"
#include "thread_utility.hpp"
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
class hid_set_report final {
public:
  hid_set_report(const hid_set_report&) = delete;

  hid_set_report(void) {
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

    auto self = static_cast<hid_set_report*>(context);
    if (!self) {
      return;
    }

    self->device_matching_callback(device);
  }

  void device_matching_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    if (!krbn::iokit_utility::is_karabiner_virtual_hid_device(device)) {
      return;
    }

    krbn::iokit_utility::log_matching_device(device);

    {
      auto kr = IOHIDDeviceOpen(device, kIOHIDOptionsTypeNone);
      if (kr != kIOReturnSuccess) {
        krbn::logger::get_logger().error("Failed to IOHIDDeviceOpen");
        return;
      }
    }

    {
      pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input keyboard_input;
      const uint8_t* report = reinterpret_cast<const uint8_t*>(&keyboard_input);

      auto kr = IOHIDDeviceSetReport(device,
                                     kIOHIDReportTypeInput,
                                     report[0],
                                     report,
                                     sizeof(keyboard_input));
      if (kr != kIOReturnSuccess) {
        krbn::logger::get_logger().error("Failed to IOHIDDeviceSetReport: {0}",
                                         krbn::iokit_utility::get_error_name(kr));
        return;
      }
    }

    {
      auto kr = IOHIDDeviceClose(device, kIOHIDOptionsTypeNone);
      if (kr != kIOReturnSuccess) {
        krbn::logger::get_logger().error("Failed to IOHIDDeviceClose");
        return;
      }
    }
  }

  static void static_device_removal_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess || !device) {
      return;
    }

    auto self = static_cast<hid_set_report*>(context);
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
  }

  IOHIDManagerRef _Nullable manager_;
};
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  hid_set_report hid_set_report;
  CFRunLoopRun();
  return 0;
}
