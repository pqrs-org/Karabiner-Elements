#include "karabiner_virtual_hid_device_methods.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <cmath>
#include <iostream>
#include <thread>

int main(int argc, const char* argv[]) {
  if (getuid() != 0) {
    std::cerr << "dispatch_keyboard_event_example requires root privilege." << std::endl;
  }

  std::cout << pqrs::karabiner_virtual_hid_device::get_kernel_extension_name() << std::endl;

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
    pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization properties;
    kr = pqrs::karabiner_virtual_hid_device_methods::initialize_virtual_hid_keyboard(connect, properties);
    if (kr != KERN_SUCCESS) {
      std::cerr << "initialize_virtual_hid_keyboard error" << std::endl;
    }

    while (true) {
      std::cout << "Checking virtual_hid_keyboard is ready..." << std::endl;

      bool ready;
      kr = pqrs::karabiner_virtual_hid_device_methods::is_virtual_hid_keyboard_ready(connect, ready);
      if (kr != KERN_SUCCESS) {
        std::cerr << "is_virtual_hid_keyboard_ready error: " << kr << std::endl;
      } else {
        if (ready) {
          std::cout << "virtual_hid_keyboard is ready." << std::endl;
          break;
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  // keyboard_input

  {
    pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization properties;
#if 0
    properties.country_code = 33;
#endif
    kr = pqrs::karabiner_virtual_hid_device_methods::initialize_virtual_hid_keyboard(connect, properties);
    if (kr != KERN_SUCCESS) {
      std::cerr << "initialize_virtual_hid_keyboard error" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  {
    pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input report;
    for (int i = 0; i < 10; ++i) {
      uint8_t key = 0;
      switch (i % 5) {
        case 0:
          key = kHIDUsage_KeyboardA;
          break;
        case 1:
          key = kHIDUsage_KeyboardB;
          break;
        case 2:
          key = kHIDUsage_KeyboardC;
          break;
        case 3:
          key = kHIDUsage_KeyboardD;
          report.modifiers.insert(pqrs::karabiner_virtual_hid_device::hid_report::modifier::right_shift);
          break;
        case 4:
          key = kHIDUsage_KeyboardE;
          break;
      }

      if (report.keys.exists(key)) {
        report.keys.erase(key);
      } else {
        report.keys.insert(key);
      }

      kr = pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect, report);
      if (kr != KERN_SUCCESS) {
        std::cerr << "post_keyboard_input_report error" << std::endl;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  // consumer_input (option+mute)

  {
    pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input report;
    report.modifiers.insert(pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_option);

    kr = pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect, report);
    if (kr != KERN_SUCCESS) {
      std::cerr << "post_keyboard_input_report error" << std::endl;
    }
  }
  {
    pqrs::karabiner_virtual_hid_device::hid_report::consumer_input report;
    report.keys.insert(kHIDUsage_Csmr_Mute);

    kr = pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect, report);
    if (kr != KERN_SUCCESS) {
      std::cerr << "post_keyboard_input_report error" << std::endl;
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // apple_vendor_top_case_input (fn+spacebar)

  {
    pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_top_case_input report;
    report.keys.insert(0x03); // kHIDUsage_AV_TopCase_KeyboardFn

    kr = pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect, report);
    if (kr != KERN_SUCCESS) {
      std::cerr << "post_keyboard_input_report error" << std::endl;
    }
  }
  {
    pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input report;
    report.keys.insert(kHIDUsage_KeyboardSpacebar);

    kr = pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect, report);
    if (kr != KERN_SUCCESS) {
      std::cerr << "post_keyboard_input_report error" << std::endl;
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // apple_vendor_keyboard_input (command+mission_control)

  {
    pqrs::karabiner_virtual_hid_device::hid_report::keyboard_input report;
    report.modifiers.insert(pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_command);

    kr = pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect, report);
    if (kr != KERN_SUCCESS) {
      std::cerr << "post_keyboard_input_report error" << std::endl;
    }
  }

  {
    pqrs::karabiner_virtual_hid_device::hid_report::apple_vendor_keyboard_input report;
    report.keys.insert(0x0010); // kHIDUsage_AppleVendorKeyboard_Expose_All

    kr = pqrs::karabiner_virtual_hid_device_methods::post_keyboard_input_report(connect, report);
    if (kr != KERN_SUCCESS) {
      std::cerr << "post_keyboard_input_report error" << std::endl;
    }
  }

  kr = pqrs::karabiner_virtual_hid_device_methods::reset_virtual_hid_keyboard(connect);
  if (kr != KERN_SUCCESS) {
    std::cerr << "reset_virtual_hid_keyboard error" << std::endl;
  }

  std::cout << std::endl;
  std::cout << "press control-c to quit" << std::endl;
  while (true) {
    std::cout << "." << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
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
