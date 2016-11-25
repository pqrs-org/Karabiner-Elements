#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <iostream>
#include <mach/mach_time.h>

namespace {
uint8_t buffer[256];

void input_report_callback(void* _Nullable context,
                           IOReturn result,
                           void* _Nullable sender,
                           IOHIDReportType type,
                           uint32_t report_id,
                           uint8_t* _Nullable report,
                           CFIndex report_length) {
  if (result != kIOReturnSuccess) {
    return;
  }

  std::cout << "report_length: " << report_length << std::endl;
  std::cout << "  report_id: " << report_id << std::endl;
  for (CFIndex i = 0; i < report_length; ++i) {
    std::cout << "  key[" << i << "]: " << static_cast<int>(report[i]) << std::endl;
  }
}

void device_matching_callback(void* _Nullable context,
                              IOReturn result,
                              void* _Nullable sender,
                              IOHIDDeviceRef _Nonnull device) {
  if (result != kIOReturnSuccess || !device) {
    return;
  }

  std::cout << "device_matching_callback" << std::endl;

  IOHIDDeviceOpen(device, kIOHIDOptionsTypeNone);
  IOHIDDeviceRegisterInputReportCallback(device, buffer, sizeof(buffer), input_report_callback, nullptr);
  IOHIDDeviceScheduleWithRunLoop(device, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
}
}

int main(int argc, const char* argv[]) {
  uint32_t usage_page = kHIDPage_GenericDesktop;
  uint32_t usage = kHIDUsage_GD_Keyboard;

  if (auto device_matching_dictionary = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                                  0,
                                                                  &kCFTypeDictionaryKeyCallBacks,
                                                                  &kCFTypeDictionaryValueCallBacks)) {
    if (auto number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage_page)) {
      CFDictionarySetValue(device_matching_dictionary, CFSTR(kIOHIDElementUsagePageKey), number);
      CFRelease(number);
    }
    if (auto number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage)) {
      CFDictionarySetValue(device_matching_dictionary, CFSTR(kIOHIDElementUsageKey), number);
      CFRelease(number);
    }

    if (auto manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone)) {
      IOHIDManagerSetDeviceMatching(manager, device_matching_dictionary);
      IOHIDManagerRegisterDeviceMatchingCallback(manager, device_matching_callback, nullptr);
      IOHIDManagerScheduleWithRunLoop(manager, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    }

    CFRelease(device_matching_dictionary);
  }

  CFRunLoopRun();

  return 0;
}
