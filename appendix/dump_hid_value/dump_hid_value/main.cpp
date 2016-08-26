#include "boost_defs.hpp"

#include "human_interface_device.hpp"
#include "iokit_utility.hpp"
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

class logger {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("dump_hid_value", true);
    }
    return *logger;
  }
};

class dump_hid_value {
public:
  dump_hid_value(void) {
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

    hids_[device] = std::make_unique<human_interface_device>(logger::get_logger(), device);
    auto& dev = hids_[device];

    logger::get_logger().info("matching device: "
                              "manufacturer:{1}, "
                              "product:{2}, "
                              "vendor_id:0x{3:x}, "
                              "product_id:0x{4:x}, "
                              "location_id:0x{5:x}, "
                              "serial_number:{6} "
                              "@ {0}",
                              __PRETTY_FUNCTION__,
                              dev->get_manufacturer(),
                              dev->get_product(),
                              dev->get_vendor_id(),
                              dev->get_product_id(),
                              dev->get_location_id(),
                              dev->get_serial_number_string());

    dev->open();
    dev->register_value_callback(boost::bind(&dump_hid_value::value_callback, this, _1, _2, _3, _4, _5, _6));
    dev->schedule();
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

    auto it = hids_.find(device);
    if (it != hids_.end()) {
      auto& dev = it->second;
      if (dev) {
        logger::get_logger().info("removal device: "
                                  "vendor_id:0x{1:x}, "
                                  "product_id:0x{2:x}, "
                                  "location_id:0x{3:x} "
                                  "@ {0}",
                                  __PRETTY_FUNCTION__,
                                  dev->get_vendor_id(),
                                  dev->get_product_id(),
                                  dev->get_location_id());

        hids_.erase(it);
      }
    }
  }

  void value_callback(human_interface_device& device,
                      IOHIDValueRef _Nonnull value,
                      IOHIDElementRef _Nonnull element,
                      uint32_t usage_page,
                      uint32_t usage,
                      CFIndex integer_value) {
    std::cout << "element" << std::endl
              << "  usage_page:0x" << std::hex << usage_page << std::endl
              << "  usage:0x" << std::hex << usage << std::endl
              << "  type:" << IOHIDElementGetType(element) << std::endl
              << "  length:" << IOHIDValueGetLength(value) << std::endl
              << "  integer_value:" << integer_value << std::endl;
  }

  IOHIDManagerRef _Nullable manager_;
  uint8_t report_[128];
  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<human_interface_device>> hids_;
};

int main(int argc, const char* argv[]) {
  dump_hid_value d;
  CFRunLoopRun();
  return 0;
}
