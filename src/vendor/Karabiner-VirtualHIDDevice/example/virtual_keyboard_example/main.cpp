#include "karabiner_virtualhiddevice_methods.hpp"
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
    std::cerr << "virtual_keyboard_example requires root privilege." << std::endl;
  }

  kern_return_t kr;
  io_connect_t connect = IO_OBJECT_NULL;
  auto service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceNameMatching(pqrs::karabiner_virtualhiddevice::get_virtual_hid_root_name()));
  if (!service) {
    std::cerr << "IOServiceGetMatchingService error" << std::endl;
    goto finish;
  }

  kr = IOServiceOpen(service, mach_task_self(), kIOHIDServerConnectType, &connect);
  if (kr != KERN_SUCCESS) {
    std::cerr << "IOServiceOpen error" << std::endl;
    goto finish;
  }

  kr = pqrs::karabiner_virtualhiddevice_methods::initialize_virtual_hid_keyboard(connect);
  if (kr != KERN_SUCCESS) {
    std::cerr << "initialize_virtual_hid_keyboard error" << std::endl;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for (int i = 0; i < 12; ++i) {
    pqrs::karabiner_virtualhiddevice::hid_report::keyboard_input report;
    switch (i % 6) {
    case 0:
      report.keys[0] = 0x04; // a
      break;
    case 1:
      report.keys[0] = 0x05; // b
      break;
    case 2:
      report.keys[0] = 0x06; // c
      break;
    case 3:
      report.keys[0] = 0x07; // d
      break;
    case 4:
      report.keys[0] = 0x08; // e
      break;
    case 5:
      // Send empty report
      break;
    }

    kr = pqrs::karabiner_virtualhiddevice_methods::post_keyboard_input_report(connect, report);
    if (kr != KERN_SUCCESS) {
      std::cerr << "post_keyboard_input_report error" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  kr = pqrs::karabiner_virtualhiddevice_methods::reset_virtual_hid_keyboard(connect);
  if (kr != KERN_SUCCESS) {
    std::cerr << "reset_virtual_hid_keyboard error" << std::endl;
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
