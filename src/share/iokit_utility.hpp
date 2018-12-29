#pragma once

#include "logger.hpp"
#include "types.hpp"
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <cstdint>
#include <pqrs/osx/iokit_hid_device.hpp>
#include <pqrs/string.hpp>
#include <string>
#include <vector>

namespace krbn {
class iokit_utility final {
public:
  static bool is_keyboard(IOHIDDeviceRef _Nonnull device) {
    if (device) {
      return IOHIDDeviceConformsTo(device, kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard);
    }
    return false;
  }

  static bool is_pointing_device(IOHIDDeviceRef _Nonnull device) {
    if (device) {
      return IOHIDDeviceConformsTo(device, kHIDPage_GenericDesktop, kHIDUsage_GD_Pointer) ||
             IOHIDDeviceConformsTo(device, kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse);
    }
    return false;
  }

  static bool is_karabiner_virtual_hid_device(IOHIDDeviceRef _Nonnull device) {
    pqrs::osx::iokit_hid_device hid_device(device);

    if (auto manufacturer = hid_device.find_manufacturer()) {
      if (*manufacturer == "pqrs.org") {
        return true;
      }
    }

    return false;
  }

  static std::vector<hid_value> make_hid_values(std::shared_ptr<std::vector<pqrs::cf::cf_ptr<IOHIDValueRef>>> values) {
    std::vector<hid_value> hid_values;

    for (const auto& v : *values) {
      hid_values.emplace_back(*v);
    }

    return hid_values;
  }

  static std::string make_device_name_for_log(device_id device_id,
                                              IOHIDDeviceRef _Nonnull device) {
    std::stringstream stream;
    pqrs::osx::iokit_hid_device hid_device(device);

    if (auto product_name = hid_device.find_product()) {
      stream << pqrs::string::trim_copy(*product_name);
    } else {
      if (auto vendor_id = hid_device.find_vendor_id()) {
        if (auto product_id = hid_device.find_product_id()) {
          stream << std::hex
                 << "(vendor_id:0x" << type_safe::get(*vendor_id)
                 << ", product_id:0x" << type_safe::get(*product_id)
                 << ")"
                 << std::dec;
        }
      }
    }

    stream << " (device_id:" << type_safe::get(device_id) << ")";
    return stream.str();
  }

  static void log_matching_device(pqrs::osx::iokit_registry_entry_id registry_entry_id,
                                  IOHIDDeviceRef _Nonnull device) {
    pqrs::osx::iokit_hid_device hid_device(device);

    logger::get_logger()->info("matching device:");
    logger::get_logger()->info("  registry_entry_id: {0}", type_safe::get(registry_entry_id));

    if (auto manufacturer = hid_device.find_manufacturer()) {
      logger::get_logger()->info("  manufacturer: {0}", *manufacturer);
    }
    if (auto product = hid_device.find_product()) {
      logger::get_logger()->info("  product: {0}", *product);
    }
    if (auto vendor_id = hid_device.find_vendor_id()) {
      logger::get_logger()->info("  vendor_id: {0}", type_safe::get(*vendor_id));
    }
    if (auto product_id = hid_device.find_product_id()) {
      logger::get_logger()->info("  product_id: {0}", type_safe::get(*product_id));
    }
    if (auto location_id = hid_device.find_location_id()) {
      logger::get_logger()->info("  location_id: {0:#x}", type_safe::get(*location_id));
    }
    if (auto serial_number = hid_device.find_serial_number()) {
      logger::get_logger()->info("  serial_number: {0}", *serial_number);
    }
    logger::get_logger()->info("  is_keyboard: {0}", is_keyboard(device));
    logger::get_logger()->info("  is_pointing_device: {0}", is_pointing_device(device));
  }
};
} // namespace krbn
