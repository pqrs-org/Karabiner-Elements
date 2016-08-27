#pragma once

#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDKeys.h>

#include <cstdint>
#include <string>
#include <vector>

class iokit_utility {
public:
  static bool get_registry_entry_id(io_registry_entry_t registry_entry, uint64_t& value) {
    // Note:
    // IORegistryEntryGetRegistryEntryID returns MACH_SEND_INVALID_DEST in callback of IOHIDManagerRegisterDeviceRemovalCallback.
    // Thus, we cannot use it as a unique id for device matching/removal.

    auto kr = IORegistryEntryGetRegistryEntryID(registry_entry, &value);
    return (kr == KERN_SUCCESS);
  }

  static bool get_string_property(io_service_t service, CFStringRef _Nonnull key, std::string& value) {
    if (!service) {
      return false;
    }

    auto property = IORegistryEntrySearchCFProperty(service,
                                                    kIOServicePlane,
                                                    key,
                                                    kCFAllocatorDefault,
                                                    kIORegistryIterateRecursively | kIORegistryIterateParents);
    if (!property) {
      return false;
    }

    if (auto p = CFStringGetCStringPtr(static_cast<CFStringRef>(property), kCFStringEncodingUTF8)) {
      value = p;
    } else {
      value.clear();
    }

    CFRelease(property);
    return true;
  }

  static std::string get_manufacturer(io_service_t service) {
    std::string value;
    get_string_property(service, CFSTR(kIOHIDManufacturerKey), value);
    return value;
  }

  static std::string get_product(io_service_t service) {
    std::string value;
    get_string_property(service, CFSTR(kIOHIDProductKey), value);
    return value;
  }

  static std::string get_serial_number(io_service_t service) {
    std::string value;
    get_string_property(service, CFSTR(kIOHIDSerialNumberKey), value);
    return value;
  }

  static CFDictionaryRef _Nonnull create_matching_dictionary(CFStringRef _Nonnull usage_page_key, uint32_t usage_page,
                                                             CFStringRef _Nonnull usage_key, uint32_t usage) {
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
      CFDictionarySetValue(device_matching_dictionary, usage_page_key, number);
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
      CFDictionarySetValue(device_matching_dictionary, usage_key, number);
      CFRelease(number);
    }

  finish:
    return device_matching_dictionary;
  }

  static CFDictionaryRef _Nonnull create_device_matching_dictionary(uint32_t usage_page, uint32_t usage) {
    return create_matching_dictionary(CFSTR(kIOHIDDeviceUsagePageKey), usage_page,
                                      CFSTR(kIOHIDDeviceUsageKey), usage);
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

  static CFDictionaryRef _Nonnull create_element_matching_dictionary(uint32_t usage_page, uint32_t usage) {
    return create_matching_dictionary(CFSTR(kIOHIDElementUsagePageKey), usage_page,
                                      CFSTR(kIOHIDElementUsageKey), usage);
  }

  static bool get_registry_entry_id(IOHIDDeviceRef _Nonnull device, uint64_t& value) {
    return get_registry_entry_id(IOHIDDeviceGetService(device), value);
  }

  static bool get_long_property(IOHIDDeviceRef _Nonnull device, CFStringRef _Nonnull key, long& value) {
    auto property = IOHIDDeviceGetProperty(device, key);
    if (!property) {
      return false;
    }

    if (CFNumberGetTypeID() != CFGetTypeID(property)) {
      return false;
    }

    return CFNumberGetValue(static_cast<CFNumberRef>(property), kCFNumberLongType, &value);
  }

  static bool get_string_property(IOHIDDeviceRef _Nonnull device, CFStringRef _Nonnull key, std::string& value) {
    auto property = IOHIDDeviceGetProperty(device, key);
    if (!property) {
      return false;
    }

    if (CFStringGetTypeID() != CFGetTypeID(property)) {
      return false;
    }

    if (auto p = CFStringGetCStringPtr(static_cast<CFStringRef>(property), kCFStringEncodingUTF8)) {
      value = p;
    } else {
      value.clear();
    }
    return true;
  }

  static long get_max_input_report_size(IOHIDDeviceRef _Nonnull device) {
    long value = 0;
    get_long_property(device, CFSTR(kIOHIDMaxInputReportSizeKey), value);
    return value;
  }

  static long get_vendor_id(IOHIDDeviceRef _Nonnull device) {
    long value = 0;
    get_long_property(device, CFSTR(kIOHIDVendorIDKey), value);
    return value;
  }

  static long get_product_id(IOHIDDeviceRef _Nonnull device) {
    long value = 0;
    get_long_property(device, CFSTR(kIOHIDProductIDKey), value);
    return value;
  }

  static long get_location_id(IOHIDDeviceRef _Nonnull device) {
    long value = 0;
    get_long_property(device, CFSTR(kIOHIDLocationIDKey), value);
    return value;
  }

  static std::string get_manufacturer(IOHIDDeviceRef _Nonnull device) {
    std::string value;
    get_string_property(device, CFSTR(kIOHIDManufacturerKey), value);
    return value;
  }

  static std::string get_product(IOHIDDeviceRef _Nonnull device) {
    std::string value;
    get_string_property(device, CFSTR(kIOHIDProductKey), value);
    return value;
  }

  static std::string get_serial_number(IOHIDDeviceRef _Nonnull device) {
    std::string value;
    get_string_property(device, CFSTR(kIOHIDSerialNumberKey), value);
    return value;
  }
};
