#include "karabiner_virtual_hid_device_methods.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <cmath>
#include <iostream>
#include <thread>

int main(int argc, const char* argv[]) {
  if (getuid() != 0) {
    std::cerr << "modifier_flag_manipulate_example requires root privilege." << std::endl;
  }

  kern_return_t kr;
  io_connect_t connect = IO_OBJECT_NULL;
  auto service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceNameMatching(pqrs::karabiner_virtual_hid_device::get_virtual_hid_root_name()));
  if (!service) {
    std::cerr << "IOServiceGetMatchingService error" << std::endl;
    goto finish;
  }

  kr = IOServiceOpen(service, mach_task_self(), kIOHIDServerConnectType, &connect);
  if (kr != KERN_SUCCESS) {
    std::cerr << "IOServiceOpen error" << std::endl;
    goto finish;
  }

  {
    std::cout << "left control by virtual_hid_keyboard (3 seconds)" << std::endl;

    kr = pqrs::karabiner_virtual_hid_device_methods::initialize_virtual_hid_keyboard(connect);
    if (kr != KERN_SUCCESS) {
      std::cerr << "initialize_virtual_hid_keyboard error" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input report;
    report.modifiers = 0x01;

    kr = pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect, report);
    if (kr != KERN_SUCCESS) {
      std::cerr << "post_keyboard_input_report error" << std::endl;
    }

    for (int i = 0; i < 3; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      std::cout << (i + 1) << std::endl;
    }

    kr = pqrs::karabiner_virtual_hid_device_methods::terminate_virtual_hid_keyboard(connect);
    if (kr != KERN_SUCCESS) {
      std::cerr << "terminate_virtual_hid_keyboard error" << std::endl;
    }
  }

  {
    std::cout << "left control by post_keyboard_event (3 seconds)" << std::endl;

    pqrs::karabiner_virtual_hid_device::keyboard_event keyboard_event;
    keyboard_event.event_type = pqrs::karabiner_virtual_hid_device::event_type::flags_changed;
    keyboard_event.key = 0x3b;
    keyboard_event.flags = 0x40001;

    kr = pqrs::karabiner_virtual_hid_device_methods::post_keyboard_event(connect, keyboard_event);
    if (kr != KERN_SUCCESS) {
      std::cerr << "post_keyboard_event error" << std::endl;
    }

    for (int i = 0; i < 3; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      std::cout << (i + 1) << std::endl;
    }

    keyboard_event.flags = 0;

    kr = pqrs::karabiner_virtual_hid_device_methods::post_keyboard_event(connect, keyboard_event);
    if (kr != KERN_SUCCESS) {
      std::cerr << "post_keyboard_event error" << std::endl;
    }
  }

  {
    std::cout << "left control by IOHIDPostEvent (3 seconds)" << std::endl;

    uid_t uid;
    if (auto user_name = SCDynamicStoreCopyConsoleUser(nullptr, &uid, nullptr)) {
      CFRelease(user_name);

      setuid(uid);

      if (auto hid_system = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceNameMatching(kIOHIDSystemClass))) {
        io_connect_t hid_system_connect = IO_OBJECT_NULL;
        kr = IOServiceOpen(hid_system, mach_task_self(), kIOHIDParamConnectType, &hid_system_connect);
        if (kr != KERN_SUCCESS) {
          std::cerr << "IOServiceOpen error" << std::endl;
        } else {
          IOGPoint location{};
          NXEventData event_data{};
          event_data.key.keyCode = 0x3b;
          uint32_t event_data_version = kNXEventDataVersion;
          IOOptionBits event_flags = 0x40001;
          IOOptionBits options = kIOHIDSetGlobalEventFlags;

          kr = IOHIDPostEvent(hid_system_connect, NX_FLAGSCHANGED, location, &event_data, event_data_version, event_flags, options);
          if (kr != KERN_SUCCESS) {
            std::cerr << "IOHIDPostEvent error" << std::endl;
          }

          for (int i = 0; i < 3; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            std::cout << (i + 1) << std::endl;
          }

          event_flags = 0;

          kr = IOHIDPostEvent(hid_system_connect, NX_FLAGSCHANGED, location, &event_data, event_data_version, event_flags, options);
          if (kr != KERN_SUCCESS) {
            std::cerr << "IOHIDPostEvent error" << std::endl;
          }
        }
      }
    }
  }

finish:
  if (connect) {
    IOServiceClose(connect);
  }
  if (service) {
    IOObjectRelease(service);
  }

  return 0;
}
