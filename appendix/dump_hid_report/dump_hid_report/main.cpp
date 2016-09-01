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

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("dump_hid_report", true);
    }
    return *logger;
  }
};

class dump_hid_report final {
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

      IOHIDManagerScheduleWithRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
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

    // Logitech Unifying Receiver sends a lot of null report. We ignore them.
    if (dev->get_manufacturer() == "Logitech" &&
        dev->get_product() == "USB Receiver") {
      return;
    }

    dev->open();
    dev->register_report_callback(boost::bind(&dump_hid_report::report_callback, this, _1, _2, _3, _4, _5), report_, sizeof(report_));
    dev->schedule();
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

  void report_callback(human_interface_device& device,
                       IOHIDReportType type,
                       uint32_t report_id,
                       uint8_t* report,
                       CFIndex report_length) {
    logger::get_logger().info("report_length: {0}", report_length);
    for (CFIndex i = 0; i < report_length; ++i) {
      std::cout << " key[" << i << "]: 0x" << std::hex << static_cast<int>(report[i]) << std::dec << std::endl;
    }
  }

  IOHIDManagerRef _Nullable manager_;
  uint8_t report_[128];
  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<human_interface_device>> hids_;
};

int main(int argc, const char* argv[]) {
  dump_hid_report d;
  CFRunLoopRun();
  return 0;
}
