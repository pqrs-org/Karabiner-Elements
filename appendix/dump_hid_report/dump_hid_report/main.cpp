// OS X headers
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <mach/mach_time.h>

#include <iostream>

#include "iokit_utility.hpp"

class dump_hid_report {
public:
  dump_hid_report(void) {
    manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager_) {
      return;
    }

    auto device_matching_dictionaries = iokit_utility::create_device_matching_dictionaries({
        std::make_pair(kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard),
    });
    if (device_matching_dictionaries) {
      IOHIDManagerSetDeviceMatchingMultiple(manager_, device_matching_dictionaries);
      CFRelease(device_matching_dictionaries);

      IOHIDManagerRegisterDeviceMatchingCallback(manager_, static_device_matching_callback, this);
      IOHIDManagerRegisterDeviceRemovalCallback(manager_, static_device_removal_callback, this);

      IOHIDManagerScheduleWithRunLoop(manager_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    }
  }

private:
  static void static_device_matching_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess || !device) {
      return;
    }

    auto self = static_cast<dump_hid_report*>(context);
    if (!self) {
      return;
    }

    self->device_matching_callback(device);
  }

  void device_matching_callback(IOHIDDeviceRef _Nonnull device) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::cout << "  " << iokit_utility::get_max_input_report_size(device) << std::endl;
    std::cout << "  " << iokit_utility::get_vendor_id(device) << std::endl;
    std::cout << "  " << iokit_utility::get_product_id(device) << std::endl;
    std::cout << "  " << iokit_utility::get_location_id(device) << std::endl;
    std::cout << "  " << iokit_utility::get_manufacturer(device) << std::endl;
    std::cout << "  " << iokit_utility::get_product(device) << std::endl;
    std::cout << "  " << iokit_utility::get_serial_number(device) << std::endl;

    // Logitech Unifying Receiver sends a lot of null report. We ignore them.
    if (iokit_utility::get_manufacturer(device) == "Logitech" &&
        iokit_utility::get_product(device) == "USB Receiver") {
      return;
    }

    IOHIDDeviceOpen(device, kIOHIDOptionsTypeNone);

    IOHIDDeviceRegisterInputReportCallback(device,
                                           report_,
                                           sizeof(report_),
                                           static_report_callback,
                                           this);

    IOHIDDeviceScheduleWithRunLoop(device, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
  }

  static void static_device_removal_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess || !device) {
      return;
    }

    auto self = static_cast<dump_hid_report*>(context);
    if (!self) {
      return;
    }

    self->device_removal_callback(device);
  }

  void device_removal_callback(IOHIDDeviceRef _Nonnull device) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    IOHIDDeviceUnscheduleFromRunLoop(device, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    IOHIDDeviceClose(device, kIOHIDOptionsTypeNone);
  }

  static void static_report_callback(void* context,
                                     IOReturn result,
                                     void* sender,
                                     IOHIDReportType type,
                                     uint32_t report_id,
                                     uint8_t* report,
                                     CFIndex report_length) {
    std::cout << "report_length: " << report_length << std::endl;
    for (CFIndex i = 0; i < report_length; ++i) {
      std::cout << " key[" << i << "]: 0x" << std::hex << static_cast<int>(report[i]) << std::dec << std::endl;
    }
  }

  IOHIDManagerRef _Nullable manager_;
  uint8_t report_[128];
};

int main(int argc, const char* argv[]) {
  dump_hid_report d;
  CFRunLoopRun();
  return 0;
}
