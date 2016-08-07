#pragma once

#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <string>

class iokit_utility {
public:
  static std::string get_serial_number(io_service_t service) {
    std::string serial_number;

    if (service) {
      if (auto property = IORegistryEntrySearchCFProperty(service,
                                                          kIOServicePlane,
                                                          CFSTR(kIOHIDSerialNumberKey),
                                                          kCFAllocatorDefault,
                                                          kIORegistryIterateRecursively | kIORegistryIterateParents)) {
        auto p = CFStringGetCStringPtr(static_cast<CFStringRef>(property), kCFStringEncodingUTF8);
        if (p) {
          serial_number = p;
        }
        CFRelease(property);
      }
    }

    return serial_number;
  }
};
