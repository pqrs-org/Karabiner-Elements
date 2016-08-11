#pragma once

#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDKeys.h>

#include <cstdint>
#include <string>
#include <vector>

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

  static CFDictionaryRef _Nonnull create_device_matching_dictionary(uint32_t usage_page, uint32_t usage) {
    auto device_matching_dictionary = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (!device_matching_dictionary) {
      goto finish;
    }

    // usage_page
    if (!usage_page) {
      goto finish;
    } else {
      auto number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage_page);
      if (!number) {
        goto finish;
      }
      CFDictionarySetValue(device_matching_dictionary, CFSTR(kIOHIDDeviceUsagePageKey), number);
      CFRelease(number);
    }

    // usage (The usage is only valid if the usage page is also defined)
    if (!usage) {
      goto finish;
    } else {
      auto number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
      if (!number) {
        goto finish;
      }
      CFDictionarySetValue(device_matching_dictionary, CFSTR(kIOHIDDeviceUsageKey), number);
      CFRelease(number);
    }

  finish:
    return device_matching_dictionary;
  }

  static CFArrayRef _Nullable create_device_matching_dictionaries(const std::vector<std::pair<uint32_t, uint32_t>>& usage_pairs) {
    auto device_matching_dictionaries = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
    if (!device_matching_dictionaries) {
      return nullptr;
    }

    for (const auto& usage_pair : usage_pairs) {
      auto device_matching_dictionary = create_device_matching_dictionary(usage_pair.first, usage_pair.second);
      if (device_matching_dictionary) {
        CFArrayAppendValue(device_matching_dictionaries, device_matching_dictionary);
        CFRelease(device_matching_dictionary);
      }
    }

    return device_matching_dictionaries;
  }
};
