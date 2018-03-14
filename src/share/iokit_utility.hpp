#pragma once

#include "boost_defs.hpp"

#include "cf_utility.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <boost/optional.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace krbn {
class iokit_utility final {
public:
  static const char* _Nonnull get_error_name(IOReturn error) {
#define KRBN_IOKIT_UTILITY_ERROR_NAME(ERROR) \
  case ERROR:                                \
    return #ERROR;

    switch (error) {
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnSuccess);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnError);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNoMemory);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNoResources);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnIPCError);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNoDevice);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNotPrivileged);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnBadArgument);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnLockedRead);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnLockedWrite);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnExclusiveAccess);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnBadMessageID);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnUnsupported);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnVMError);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnInternalError);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnIOError);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnCannotLock);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNotOpen);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNotReadable);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNotWritable);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNotAligned);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnBadMedia);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnStillOpen);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnRLDError);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnDMAError);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnBusy);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnTimeout);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnOffline);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNotReady);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNotAttached);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNoChannels);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNoSpace);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnPortExists);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnCannotWire);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNoInterrupt);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNoFrames);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnMessageTooLarge);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNotPermitted);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNoPower);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNoMedia);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnUnformattedMedia);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnUnsupportedMode);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnUnderrun);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnOverrun);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnDeviceError);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNoCompletion);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnAborted);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNoBandwidth);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNotResponding);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnIsoTooOld);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnIsoTooNew);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnNotFound);
      KRBN_IOKIT_UTILITY_ERROR_NAME(kIOReturnInvalid);
      default:
        return "unknown error";
    }

#undef KRBN_IOKIT_UTILITY_ERROR_NAME
  }

  static boost::optional<uint64_t> find_registry_entry_id(io_registry_entry_t registry_entry) {
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

  static boost::optional<std::string> find_string_property(io_service_t service, CFStringRef _Nonnull key) {
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

  static boost::optional<std::string> find_manufacturer(io_service_t service) {
    return find_string_property(service, CFSTR(kIOHIDManufacturerKey));
  }

  static boost::optional<std::string> find_product(io_service_t service) {
    return find_string_property(service, CFSTR(kIOHIDProductKey));
  }

  static boost::optional<std::string> find_serial_number(io_service_t service) {
    return find_string_property(service, CFSTR(kIOHIDSerialNumberKey));
  }

  static CFDictionaryRef _Nullable create_matching_dictionary(CFStringRef _Nonnull usage_page_key, hid_usage_page usage_page,
                                                              CFStringRef _Nonnull usage_key, hid_usage usage) {
    if (auto device_matching_dictionary = cf_utility::create_cfmutabledictionary()) {
      // usage_page
      if (usage_page != hid_usage_page::zero) {
        if (auto number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage_page)) {
          CFDictionarySetValue(device_matching_dictionary, usage_page_key, number);
          CFRelease(number);
        }
      }

      // usage (The usage is only valid if the usage page is also defined)
      if (usage != hid_usage::zero) {
        if (auto number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage)) {
          CFDictionarySetValue(device_matching_dictionary, usage_key, number);
          CFRelease(number);
        }
      }

      return device_matching_dictionary;
    }

    return nullptr;
  }

  static CFDictionaryRef _Nullable create_device_matching_dictionary(hid_usage_page usage_page, hid_usage usage) {
    return create_matching_dictionary(CFSTR(kIOHIDDeviceUsagePageKey), usage_page,
                                      CFSTR(kIOHIDDeviceUsageKey), usage);
  }

  static CFArrayRef _Nullable create_device_matching_dictionaries(const std::vector<std::pair<hid_usage_page, hid_usage>>& usage_pairs) {
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

  static CFDictionaryRef _Nullable create_element_matching_dictionary(hid_usage_page usage_page, hid_usage usage) {
    return create_matching_dictionary(CFSTR(kIOHIDElementUsagePageKey), usage_page,
                                      CFSTR(kIOHIDElementUsageKey), usage);
  }

  static boost::optional<uint64_t> find_registry_entry_id(IOHIDDeviceRef _Nonnull device) {
    return find_registry_entry_id(IOHIDDeviceGetService(device));
  }

  static boost::optional<long> find_long_property(IOHIDDeviceRef _Nonnull device, CFStringRef _Nonnull key) {
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

  static boost::optional<std::string> find_string_property(IOHIDDeviceRef _Nonnull device, CFStringRef _Nonnull key) {
    auto property = IOHIDDeviceGetProperty(device, key);
    return cf_utility::to_string(property);
  }

  static boost::optional<long> find_max_input_report_size(IOHIDDeviceRef _Nonnull device) {
    return find_long_property(device, CFSTR(kIOHIDMaxInputReportSizeKey));
  }

  static vendor_id find_vendor_id(IOHIDDeviceRef _Nonnull device) {
    if (auto value = find_long_property(device, CFSTR(kIOHIDVendorIDKey))) {
      return static_cast<vendor_id>(*value);
    }
    return vendor_id::zero;
  }

  static product_id find_product_id(IOHIDDeviceRef _Nonnull device) {
    if (auto value = find_long_property(device, CFSTR(kIOHIDProductIDKey))) {
      return static_cast<product_id>(*value);
    }
    return product_id::zero;
  }

  static boost::optional<location_id> find_location_id(IOHIDDeviceRef _Nonnull device) {
    if (auto value = find_long_property(device, CFSTR(kIOHIDLocationIDKey))) {
      return static_cast<location_id>(*value);
    }
    return boost::none;
  }

  static boost::optional<std::string> find_manufacturer(IOHIDDeviceRef _Nonnull device) {
    return find_string_property(device, CFSTR(kIOHIDManufacturerKey));
  }

  static boost::optional<std::string> find_product(IOHIDDeviceRef _Nonnull device) {
    return find_string_property(device, CFSTR(kIOHIDProductKey));
  }

  static boost::optional<std::string> find_serial_number(IOHIDDeviceRef _Nonnull device) {
    return find_string_property(device, CFSTR(kIOHIDSerialNumberKey));
  }

  static boost::optional<std::string> find_transport(IOHIDDeviceRef _Nonnull device) {
    return find_string_property(device, CFSTR(kIOHIDTransportKey));
  }

  static bool is_keyboard(IOHIDDeviceRef _Nonnull device) {
    return IOHIDDeviceConformsTo(device, kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard);
  }

  static bool is_pointing_device(IOHIDDeviceRef _Nonnull device) {
    return IOHIDDeviceConformsTo(device, kHIDPage_GenericDesktop, kHIDUsage_GD_Pointer) ||
           IOHIDDeviceConformsTo(device, kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse);
  }

  static bool is_karabiner_virtual_hid_device(IOHIDDeviceRef _Nonnull device) {
    if (auto manufacturer = find_manufacturer(device)) {
      if (*manufacturer == "pqrs.org") {
        return true;
      }
    }

    return false;
  }

  static void log_matching_device(IOHIDDeviceRef _Nonnull device) {
    logger::get_logger().info("matching device:");
    if (auto manufacturer = find_manufacturer(device)) {
      logger::get_logger().info("  manufacturer: {0}", *manufacturer);
    }
    if (auto product = find_product(device)) {
      logger::get_logger().info("  product: {0}", *product);
    }
    logger::get_logger().info("  vendor_id: {0}", static_cast<uint32_t>(find_vendor_id(device)));
    logger::get_logger().info("  product_id: {0}", static_cast<uint32_t>(find_product_id(device)));
    if (auto location_id = find_location_id(device)) {
      logger::get_logger().info("  location_id: {0:#x}", static_cast<uint32_t>(*location_id));
    }
    if (auto serial_number = find_serial_number(device)) {
      logger::get_logger().info("  serial_number: {0}", *serial_number);
    }
    if (auto registry_entry_id = find_registry_entry_id(device)) {
      logger::get_logger().info("  registry_entry_id: {0}", *registry_entry_id);
    }
    logger::get_logger().info("  is_keyboard: {0}", is_keyboard(device));
    logger::get_logger().info("  is_pointing_device: {0}", is_pointing_device(device));
  }

  static void log_removal_device(IOHIDDeviceRef _Nonnull device) {
    logger::get_logger().info("removal device:");
    logger::get_logger().info("  vendor_id: {0}", static_cast<uint32_t>(find_vendor_id(device)));
    logger::get_logger().info("  product_id: {0}", static_cast<uint32_t>(find_product_id(device)));
    if (auto location_id = find_location_id(device)) {
      logger::get_logger().info("  location_id: {0:#x}", static_cast<uint32_t>(*location_id));
    }
  }
};
} // namespace krbn
