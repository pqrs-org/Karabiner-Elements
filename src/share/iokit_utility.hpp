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
  static boost::optional<registry_entry_id> find_registry_entry_id(io_registry_entry_t registry_entry) {
    // Note:
    // IORegistryEntryGetRegistryEntryID returns MACH_SEND_INVALID_DEST in callback of IOHIDManagerRegisterDeviceRemovalCallback.
    // Thus, we cannot use it as a unique id for device matching/removal.

    uint64_t value;
    auto kr = IORegistryEntryGetRegistryEntryID(registry_entry, &value);
    if (kr != KERN_SUCCESS) {
      return boost::none;
    }
    return registry_entry_id(value);
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

  static IOHIDDeviceRef _Nullable create_hid_device(IOHIDDeviceRef _Nonnull device) {
    if (auto service = IOHIDDeviceGetService(device)) {
      return IOHIDDeviceCreate(kCFAllocatorDefault, service);
    }
    return nullptr;
  }

  static boost::optional<registry_entry_id> find_registry_entry_id(IOHIDDeviceRef _Nonnull device) {
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
    return vendor_id(0);
  }

  static product_id find_product_id(IOHIDDeviceRef _Nonnull device) {
    if (auto value = find_long_property(device, CFSTR(kIOHIDProductIDKey))) {
      return static_cast<product_id>(*value);
    }
    return product_id(0);
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
      logger::get_logger().info("  registry_entry_id: {0}", static_cast<uint64_t>(*registry_entry_id));
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
