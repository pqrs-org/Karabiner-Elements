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

    hids_[device] = std::make_unique<krbn::human_interface_device>(device);
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

    krbn::iokit_utility::log_removal_device(device);

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
                      << queued_event.get_event_type()
                      << std::endl;
          }
          break;

        case krbn::event_queue::queued_event::event::type::consumer_key_code:
          if (auto consumer_key_code = queued_event.get_event().get_consumer_key_code()) {
            std::cout << "ConsumerKey: " << std::dec << static_cast<uint32_t>(*consumer_key_code) << " "
                      << queued_event.get_event_type()
                      << std::endl;
          }
          break;

        case krbn::event_queue::queued_event::event::type::pointing_button:
          if (auto pointing_button = queued_event.get_event().get_pointing_button()) {
            std::cout << "Button: " << std::dec << static_cast<uint32_t>(*pointing_button) << " "
                      << queued_event.get_event_type()
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

        case krbn::event_queue::queued_event::event::type::shell_command:
          std::cout << "shell_command" << std::endl;
          break;

        case krbn::event_queue::queued_event::event::type::device_keys_are_released:
          std::cout << "device_keys_are_released for " << device.get_name_for_log() << " (" << device.get_device_id() << ")" << std::endl;
          break;

        case krbn::event_queue::queued_event::event::type::device_pointing_buttons_are_released:
          std::cout << "device_pointing_buttons_are_released for " << device.get_name_for_log() << " (" << device.get_device_id() << ")" << std::endl;
          break;

        case krbn::event_queue::queued_event::event::type::device_ungrabbed:
          std::cout << "device_ungrabbed for " << device.get_name_for_log() << " (" << device.get_device_id() << ")" << std::endl;
          break;

        case krbn::event_queue::queued_event::event::type::caps_lock_state_changed:
          if (auto integer_value = queued_event.get_event().get_integer_value()) {
            std::cout << "caps_lock_state_changed " << *integer_value << std::endl;
          }
          break;

        case krbn::event_queue::queued_event::event::type::event_from_ignored_device:
          std::cout << "event_from_ignored_device from " << device.get_name_for_log() << " (" << device.get_device_id() << ")" << std::endl;
          break;

        case krbn::event_queue::queued_event::event::type::frontmost_application_changed:
          std::cout << "frontmost_application_changed "
                    << queued_event.get_event().get_frontmost_application_bundle_identifier() << " "
                    << queued_event.get_event().get_frontmost_application_file_path() << std::endl;
          break;

        case krbn::event_queue::queued_event::event::type::set_variable:
          std::cout << "set_variable" << std::endl;
          break;

        case krbn::event_queue::queued_event::event::type::set_inputsource:
          std::cout << "set_inputsource" << std::endl;
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
