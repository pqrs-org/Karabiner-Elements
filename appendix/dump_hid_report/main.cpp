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

namespace {
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
  dump_hid_report(const dump_hid_report&) = delete;

  dump_hid_report(void) {
    manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager_) {
      return;
    }

    auto device_matching_dictionaries = krbn::iokit_utility::create_device_matching_dictionaries({
        std::make_pair(kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard),
        std::make_pair(kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse),
        std::make_pair(kHIDPage_GenericDesktop, kHIDUsage_GD_Pointer),
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

    krbn::iokit_utility::log_matching_device(logger::get_logger(), device);

    hids_[device] = std::make_unique<krbn::human_interface_device>(logger::get_logger(), device);
    auto& dev = hids_[device];

    dev->open();
    dev->register_report_callback(boost::bind(&dump_hid_report::report_callback, this, _1, _2, _3, _4, _5));
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

    krbn::iokit_utility::log_removal_device(logger::get_logger(), device);

    auto it = hids_.find(device);
    if (it != hids_.end()) {
      auto& dev = it->second;
      if (dev) {
        hids_.erase(it);
      }
    }
  }

  void report_callback(krbn::human_interface_device& device,
                       IOHIDReportType type,
                       uint32_t report_id,
                       uint8_t* report,
                       CFIndex report_length) {
    // Logitech Unifying Receiver sends a lot of null report. We ignore them.
    if (auto manufacturer = device.get_manufacturer()) {
      if (auto product = device.get_product()) {
        if (*manufacturer == "Logitech" && *product == "USB Receiver") {
          if (report_id == 0) {
            return;
          }
        }
      }
    }

    logger::get_logger().info("report_length: {0}", report_length);
    std::cout << "  report_id: " << std::dec << report_id << std::endl;
    for (CFIndex i = 0; i < report_length; ++i) {
      std::cout << "  key[" << i << "]: 0x" << std::hex << static_cast<int>(report[i]) << std::dec << std::endl;
    }
  }

  IOHIDManagerRef _Nullable manager_;
  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<krbn::human_interface_device>> hids_;
};
}

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  dump_hid_report d;
  CFRunLoopRun();
  return 0;
}
