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
    dev->set_value_callback(boost::bind(&dump_hid_value::value_callback, this, _1, _2));
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
                      krbn::event_queue& event_queue) {
    for (const auto& queued_event : event_queue.get_events()) {
      switch (queued_event.get_event().get_type()) {
        case krbn::event_queue::queued_event::event::type::key_code:
          if (auto key_code = queued_event.get_event().get_key_code()) {
            std::cout << "Key: " << std::dec << static_cast<uint32_t>(*key_code) << " "
                      << (queued_event.get_event_type() == krbn::event_type::key_down ? "(down)" : "(up)")
                      << std::endl;
          }
          break;

        case krbn::event_queue::queued_event::event::type::pointing_button:
          if (auto pointing_button = queued_event.get_event().get_pointing_button()) {
            std::cout << "Button: " << std::dec << static_cast<uint32_t>(*pointing_button) << " "
                      << (queued_event.get_event_type() == krbn::event_type::key_down ? "(down)" : "(up)")
                      << std::endl;
          }
          break;

        case krbn::event_queue::queued_event::event::type::pointing_x:
        case krbn::event_queue::queued_event::event::type::pointing_y:
        case krbn::event_queue::queued_event::event::type::pointing_vertical_wheel:
        case krbn::event_queue::queued_event::event::type::pointing_horizontal_wheel:
          if (auto integer_value = queued_event.get_event().get_integer_value()) {
            switch (queued_event.get_event().get_type()) {
              case krbn::event_queue::queued_event::event::type::pointing_x:
                std::cout << "Pointing X: ";
                break;
              case krbn::event_queue::queued_event::event::type::pointing_y:
                std::cout << "Pointing Y: ";
                break;
              case krbn::event_queue::queued_event::event::type::pointing_vertical_wheel:
                std::cout << "Vertical Wheel: ";
                break;
              case krbn::event_queue::queued_event::event::type::pointing_horizontal_wheel:
                std::cout << "Horizontal Wheel: ";
                break;
              default:
                break;
            }

            std::cout << std::dec << *integer_value << std::endl;
          }
          break;
      }
    }

    event_queue.clear_events();
  }

  IOHIDManagerRef _Nullable manager_;
  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<krbn::human_interface_device>> hids_;
  krbn::event_queue event_queue_;
};
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  dump_hid_value d;
  CFRunLoopRun();
  return 0;
}
