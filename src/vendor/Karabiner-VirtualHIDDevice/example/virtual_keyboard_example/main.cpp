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
#if 0
    properties.keyboard_type = pqrs::karabiner_virtual_hid_device::properties::keyboard_type::jis;
#else
    properties.keyboard_type = pqrs::karabiner_virtual_hid_device::properties::keyboard_type::ansi;
#endif
#if 0
    properties.caps_lock_delay_milliseconds = pqrs::karabiner_virtual_hid_device::milliseconds(300);
#endif
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

  // ----------------------------------------

  for (int i = 0; i < 24; ++i) {
    pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;

    switch (i % 12) {
    case 0:
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardA);
      keyboard_event.value = 1;
      break;
    case 1:
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardA);
      keyboard_event.value = 0;
      break;
    case 2:
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardOpenBracket);
      keyboard_event.value = 1;
      break;
    case 3:
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardOpenBracket);
      keyboard_event.value = 0;
      break;
    case 4:
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardRightControl);
      keyboard_event.value = 1;
      break;
    case 5:
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardRightControl);
      keyboard_event.value = 0;
      break;
    case 6:
      keyboard_event.usage_page = pqrs::karabiner_virtual_hid_device::usage_page::apple_vendor_top_case;
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage::av_top_case_keyboard_fn;
      keyboard_event.value = 1;
      break;
    case 7:
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardQ);
      keyboard_event.value = 1;
      break;
    case 8:
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardQ);
      keyboard_event.value = 0;
      break;
    case 9:
      keyboard_event.usage_page = pqrs::karabiner_virtual_hid_device::usage_page::apple_vendor_top_case;
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage::av_top_case_keyboard_fn;
      keyboard_event.value = 0;
      break;
    case 10:
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardCapsLock);
      keyboard_event.value = 1;
      break;
    case 11:
      keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardCapsLock);
      keyboard_event.value = 0;
      break;
    }

    kr = pqrs::karabiner_virtual_hid_device_methods::dispatch_keyboard_event(connect, keyboard_event);
    if (kr != KERN_SUCCESS) {
      std::cerr << "dispatch_keyboard_event error" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // ----------------------------------------
  // clear_keyboard_modifier_flags

  {
    pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
    keyboard_event.usage_page = pqrs::karabiner_virtual_hid_device::usage_page::keyboard_or_keypad;
    keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage::left_shift;
    keyboard_event.value = 1;
    kr = pqrs::karabiner_virtual_hid_device_methods::dispatch_keyboard_event(connect, keyboard_event);
    if (kr != KERN_SUCCESS) {
      std::cerr << "dispatch_keyboard_event error" << std::endl;
    }

    keyboard_event.usage_page = pqrs::karabiner_virtual_hid_device::usage_page::apple_vendor_top_case;
    keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage::av_top_case_keyboard_fn;
    keyboard_event.value = 1;
    kr = pqrs::karabiner_virtual_hid_device_methods::dispatch_keyboard_event(connect, keyboard_event);
    if (kr != KERN_SUCCESS) {
      std::cerr << "dispatch_keyboard_event error" << std::endl;
    }

    kr = pqrs::karabiner_virtual_hid_device_methods::clear_keyboard_modifier_flags(connect);
    if (kr != KERN_SUCCESS) {
      std::cerr << "clear_keyboard_modifier_flags error" << std::endl;
    }

    keyboard_event.usage_page = pqrs::karabiner_virtual_hid_device::usage_page::keyboard_or_keypad;
    keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardC);
    keyboard_event.value = 1;
    kr = pqrs::karabiner_virtual_hid_device_methods::dispatch_keyboard_event(connect, keyboard_event);
    if (kr != KERN_SUCCESS) {
      std::cerr << "dispatch_keyboard_event error" << std::endl;
    }

    keyboard_event.value = 0;
    kr = pqrs::karabiner_virtual_hid_device_methods::dispatch_keyboard_event(connect, keyboard_event);
    if (kr != KERN_SUCCESS) {
      std::cerr << "dispatch_keyboard_event error" << std::endl;
    }
  }

  // ----------------------------------------
  // key repeat
  {
    pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
    keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage(kHIDUsage_KeyboardC);
    keyboard_event.value = 1;
    kr = pqrs::karabiner_virtual_hid_device_methods::dispatch_keyboard_event(connect, keyboard_event);
    if (kr != KERN_SUCCESS) {
      std::cerr << "dispatch_keyboard_event error" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  kr = pqrs::karabiner_virtual_hid_device_methods::reset_virtual_hid_keyboard(connect);
  if (kr != KERN_SUCCESS) {
    std::cerr << "reset_virtual_hid_keyboard error" << std::endl;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::cout << std::endl;

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // ----------------------------------------
  // consumer
  {
    pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
    keyboard_event.usage_page = pqrs::karabiner_virtual_hid_device::usage_page::consumer;
    keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage::csmr_volume_decrement;
    keyboard_event.value = 1;

    kr = pqrs::karabiner_virtual_hid_device_methods::dispatch_keyboard_event(connect, keyboard_event);
    if (kr != KERN_SUCCESS) {
      std::cerr << "dispatch_keyboard_event error" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    keyboard_event.value = 0;
    kr = pqrs::karabiner_virtual_hid_device_methods::dispatch_keyboard_event(connect, keyboard_event);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  // ----------------------------------------
  // apple_vendor_keyboard

  {
    pqrs::karabiner_virtual_hid_device::hid_event_service::keyboard_event keyboard_event;
    keyboard_event.usage_page = pqrs::karabiner_virtual_hid_device::usage_page::apple_vendor_keyboard;
    keyboard_event.usage = pqrs::karabiner_virtual_hid_device::usage::apple_vendor_keyboard_expose_all;
    keyboard_event.value = 1;

    kr = pqrs::karabiner_virtual_hid_device_methods::dispatch_keyboard_event(connect, keyboard_event);
    if (kr != KERN_SUCCESS) {
      std::cerr << "dispatch_keyboard_event error" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    keyboard_event.value = 0;
    kr = pqrs::karabiner_virtual_hid_device_methods::dispatch_keyboard_event(connect, keyboard_event);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    keyboard_event.value = 1;
    kr = pqrs::karabiner_virtual_hid_device_methods::dispatch_keyboard_event(connect, keyboard_event);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    keyboard_event.value = 0;
    kr = pqrs::karabiner_virtual_hid_device_methods::dispatch_keyboard_event(connect, keyboard_event);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
