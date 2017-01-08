#pragma once

#include "boost_defs.hpp"

#include "cf_utility.hpp"
#include "types.hpp"
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <boost/optional.hpp>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

class iokit_utility final {
public:
  static boost::optional<uint64_t> get_registry_entry_id(io_registry_entry_t registry_entry) {
    // Note:
    // IORegistryEntryGetRegistryEntryID returns MACH_SEND_INVALID_DEST in callback of IOHIDManagerRegisterDeviceRemovalCallback.
    // Thus, we cannot use it as a unique id for device matching/removal.

    uint64_t value;
    auto kr = IORegistryEntryGetRegistryEntryID(registry_entry, &value);
    if (kr != KERN_SUCCESS) {
      return boost::none;
    }
    return value;
  }

  static boost::optional<std::string> get_string_property(io_service_t service, CFStringRef _Nonnull key) {
    if (!service) {
      return boost::none;
    }

    if (auto property = IORegistryEntrySearchCFProperty(service,
                                                        kIOServicePlane,
                                                        key,
                                                        kCFAllocatorDefault,
                                                        kIORegistryIterateRecursively | kIORegistryIterateParents)) {
      auto value = cf_utility::to_string(property);
      CFRelease(property);
      return value;
    }

    return boost::none;
  }

  static boost::optional<std::string> get_manufacturer(io_service_t service) {
    return get_string_property(service, CFSTR(kIOHIDManufacturerKey));
  }

  static boost::optional<std::string> get_product(io_service_t service) {
    return get_string_property(service, CFSTR(kIOHIDProductKey));
  }

  static boost::optional<std::string> get_serial_number(io_service_t service) {
    return get_string_property(service, CFSTR(kIOHIDSerialNumberKey));
  }

  static CFDictionaryRef _Nullable create_matching_dictionary(CFStringRef _Nonnull usage_page_key, uint32_t usage_page,
                                                              CFStringRef _Nonnull usage_key, uint32_t usage) {
    if (auto device_matching_dictionary = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                                    0,
                                                                    &kCFTypeDictionaryKeyCallBacks,
                                                                    &kCFTypeDictionaryValueCallBacks)) {
      // usage_page
      if (usage_page) {
        if (auto number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage_page)) {
          CFDictionarySetValue(device_matching_dictionary, usage_page_key, number);
          CFRelease(number);
        }
      }

      // usage (The usage is only valid if the usage page is also defined)
      if (usage) {
        if (auto number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage)) {
          CFDictionarySetValue(device_matching_dictionary, usage_key, number);
          CFRelease(number);
        }
      }

      return device_matching_dictionary;
    }

    return nullptr;
  }

  static CFDictionaryRef _Nullable create_device_matching_dictionary(uint32_t usage_page, uint32_t usage) {
    return create_matching_dictionary(CFSTR(kIOHIDDeviceUsagePageKey), usage_page,
                                      CFSTR(kIOHIDDeviceUsageKey), usage);
  }

  static CFArrayRef _Nullable create_device_matching_dictionaries(const std::vector<std::pair<uint32_t, uint32_t>>& usage_pairs) {
    auto device_matching_dictionaries = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
    if (!device_matching_dictionaries) {
      return nullptr;
    }

    for (const auto& usage_pair : usage_pairs) {
      if (auto device_matching_dictionary = create_device_matching_dictionary(usage_pair.first, usage_pair.second)) {
        CFArrayAppendValue(device_matching_dictionaries, device_matching_dictionary);
        CFRelease(device_matching_dictionary);
      }
    }

    return device_matching_dictionaries;
  }

  static CFDictionaryRef _Nullable create_element_matching_dictionary(uint32_t usage_page, uint32_t usage) {
    return create_matching_dictionary(CFSTR(kIOHIDElementUsagePageKey), usage_page,
                                      CFSTR(kIOHIDElementUsageKey), usage);
  }

  static boost::optional<uint64_t> get_registry_entry_id(IOHIDDeviceRef _Nonnull device) {
    return get_registry_entry_id(IOHIDDeviceGetService(device));
  }

  static boost::optional<long> get_long_property(IOHIDDeviceRef _Nonnull device, CFStringRef _Nonnull key) {
    auto property = IOHIDDeviceGetProperty(device, key);
    if (!property) {
      return boost::none;
    }

    if (CFNumberGetTypeID() != CFGetTypeID(property)) {
      return boost::none;
    }

    long value = 0;
    if (!CFNumberGetValue(static_cast<CFNumberRef>(property), kCFNumberLongType, &value)) {
      return boost::none;
    }

    return value;
  }

  static boost::optional<std::string> get_string_property(IOHIDDeviceRef _Nonnull device, CFStringRef _Nonnull key) {
    auto property = IOHIDDeviceGetProperty(device, key);
    return cf_utility::to_string(property);
  }

  static boost::optional<long> get_max_input_report_size(IOHIDDeviceRef _Nonnull device) {
    return get_long_property(device, CFSTR(kIOHIDMaxInputReportSizeKey));
  }

  static boost::optional<krbn::vendor_id> get_vendor_id(IOHIDDeviceRef _Nonnull device) {
    if (auto value = get_long_property(device, CFSTR(kIOHIDVendorIDKey))) {
      return static_cast<krbn::vendor_id>(*value);
    }
    return boost::none;
  }

  static boost::optional<krbn::product_id> get_product_id(IOHIDDeviceRef _Nonnull device) {
    if (auto value = get_long_property(device, CFSTR(kIOHIDProductIDKey))) {
      return static_cast<krbn::product_id>(*value);
    }
    return boost::none;
  }

  static boost::optional<krbn::location_id> get_location_id(IOHIDDeviceRef _Nonnull device) {
    if (auto value = get_long_property(device, CFSTR(kIOHIDLocationIDKey))) {
      return static_cast<krbn::location_id>(*value);
    }
    return boost::none;
  }

  static boost::optional<std::string> get_manufacturer(IOHIDDeviceRef _Nonnull device) {
    return get_string_property(device, CFSTR(kIOHIDManufacturerKey));
  }

  static boost::optional<std::string> get_product(IOHIDDeviceRef _Nonnull device) {
    return get_string_property(device, CFSTR(kIOHIDProductKey));
  }

  static boost::optional<std::string> get_serial_number(IOHIDDeviceRef _Nonnull device) {
    return get_string_property(device, CFSTR(kIOHIDSerialNumberKey));
  }

  static boost::optional<std::string> get_transport(IOHIDDeviceRef _Nonnull device) {
    return get_string_property(device, CFSTR(kIOHIDTransportKey));
  }

  static void log_matching_device(spdlog::logger& logger,
                                  IOHIDDeviceRef _Nonnull device) {
    logger.info("matching device:");
    if (auto manufacturer = get_manufacturer(device)) {
      logger.info("  manufacturer: {0}", *manufacturer);
    }
    if (auto product = get_product(device)) {
      logger.info("  product: {0}", *product);
    }
    if (auto vendor_id = get_vendor_id(device)) {
      logger.info("  vendor_id: {0:#x}", static_cast<uint32_t>(*vendor_id));
    }
    if (auto product_id = get_product_id(device)) {
      logger.info("  product_id: {0:#x}", static_cast<uint32_t>(*product_id));
    }
    if (auto location_id = get_location_id(device)) {
      logger.info("  location_id: {0:#x}", static_cast<uint32_t>(*location_id));
    }
    if (auto serial_number = get_serial_number(device)) {
      logger.info("  serial_number: {0}", *serial_number);
    }
    if (auto registry_entry_id = get_registry_entry_id(device)) {
      logger.info("  registry_entry_id: {0}", *registry_entry_id);
    }
  }

  static void log_removal_device(spdlog::logger& logger,
                                 IOHIDDeviceRef _Nonnull device) {
    logger.info("removal device:");
    if (auto vendor_id = get_vendor_id(device)) {
      logger.info("  vendor_id: {0:#x}", static_cast<uint32_t>(*vendor_id));
    }
    if (auto product_id = get_product_id(device)) {
      logger.info("  product_id: {0:#x}", static_cast<uint32_t>(*product_id));
    }
    if (auto location_id = get_location_id(device)) {
      logger.info("  location_id: {0:#x}", static_cast<uint32_t>(*location_id));
    }
  }
};
