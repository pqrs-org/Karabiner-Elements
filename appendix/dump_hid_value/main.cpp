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
      logger = spdlog::stdout_color_mt("dump_hid_value");
    }
    return *logger;
  }
};

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

    krbn::iokit_utility::log_matching_device(logger::get_logger(), device);

    hids_[device] = std::make_unique<krbn::human_interface_device>(logger::get_logger(), device);
    auto& dev = hids_[device];
    dev->set_value_callback(boost::bind(&dump_hid_value::value_callback, this, _1, _2, _3, _4, _5, _6));
    dev->observe();
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

    krbn::iokit_utility::log_removal_device(logger::get_logger(), device);

    auto it = hids_.find(device);
    if (it != hids_.end()) {
      auto& dev = it->second;
      if (dev) {
        hids_.erase(it);
      }
    }
  }

  void value_callback(krbn::human_interface_device& device,
                      IOHIDValueRef _Nonnull value,
                      IOHIDElementRef _Nonnull element,
                      krbn::hid_usage_page usage_page,
                      krbn::hid_usage usage,
                      CFIndex integer_value) {
    if (usage_page == krbn::hid_usage_page::button) {
      std::cout << "Button: " << std::dec << static_cast<uint32_t>(usage) << " (" << integer_value << ")" << std::endl;
      return;
    }
    if (usage_page == krbn::hid_usage_page::generic_desktop && usage == krbn::hid_usage::gd_x) {
      std::cout << "Pointing X: " << std::dec << integer_value << std::endl;
      return;
    }
    if (usage_page == krbn::hid_usage_page::generic_desktop && usage == krbn::hid_usage::gd_y) {
      std::cout << "Pointing Y: " << std::dec << integer_value << std::endl;
      return;
    }
    if (usage_page == krbn::hid_usage_page::generic_desktop && usage == krbn::hid_usage::gd_z) {
      std::cout << "Pointing Z: " << std::dec << integer_value << std::endl;
      return;
    }
    if (usage_page == krbn::hid_usage_page::generic_desktop && usage == krbn::hid_usage::gd_wheel) {
      std::cout << "Wheel: " << std::dec << integer_value << std::endl;
      return;
    }
    if (usage_page == krbn::hid_usage_page::consumer && usage == krbn::hid_usage::csmr_acpan) {
      std::cout << "Horizontal Wheel: " << std::dec << integer_value << std::endl;
      return;
    }

    std::cout << "element" << std::endl
              << "  usage_page:0x" << std::hex << static_cast<uint32_t>(usage_page) << std::endl
              << "  usage:0x" << std::hex << static_cast<uint32_t>(usage) << std::endl
              << "  type:" << IOHIDElementGetType(element) << std::endl
              << "  length:" << IOHIDValueGetLength(value) << std::endl
              << "  integer_value:" << integer_value << std::endl;
  }

  IOHIDManagerRef _Nullable manager_;
  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<krbn::human_interface_device>> hids_;
};
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  dump_hid_value d;
  CFRunLoopRun();
  return 0;
}
