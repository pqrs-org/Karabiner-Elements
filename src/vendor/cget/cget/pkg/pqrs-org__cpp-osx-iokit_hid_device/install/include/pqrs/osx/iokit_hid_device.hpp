#pragma once

// pqrs::osx::iokit_hid_device v4.2

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <optional>
#include <pqrs/cf/array.hpp>
#include <pqrs/cf/string.hpp>
#include <pqrs/hid.hpp>
#include <pqrs/osx/iokit_object_ptr.hpp>
#include <pqrs/osx/iokit_registry_entry.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace pqrs {
namespace osx {
class iokit_hid_device final {
public:
  iokit_hid_device(IOHIDDeviceRef device)
      : device_(device),
        service_(device ? IOHIDDeviceGetService(device) : IO_OBJECT_NULL),
        registry_entry_(service_) {
  }

  virtual ~iokit_hid_device(void) {
  }

  cf::cf_ptr<IOHIDDeviceRef> get_device(void) const {
    return device_;
  }

  iokit_object_ptr get_service(void) const {
    return service_;
  }

  iokit_registry_entry get_registry_entry(void) const {
    return registry_entry_;
  }

  // Note:
  // Input Monitoring permission user approval is required since macOS Catalina (10.15).
  bool conforms_to(hid::usage_page::value_t usage_page, hid::usage::value_t usage) const {
    if (device_) {
      return IOHIDDeviceConformsTo(*device_,
                                   type_safe::get(usage_page),
                                   type_safe::get(usage));
    }

    return false;
  }

  std::optional<int64_t> find_int64_property(CFStringRef key, bool include_parents = false) const {
    if (auto property = registry_entry_.find_int64_property(key)) {
      return property;
    }

    if (include_parents) {
      for (const auto& entry : get_parent_registry_entries(registry_entry_)) {
        if (auto property = entry.find_int64_property(key)) {
          return property;
        }
      }
    }

    return std::nullopt;
  }

  std::optional<std::string> find_string_property(CFStringRef key, bool include_parents = false) const {
    if (auto property = registry_entry_.find_string_property(key)) {
      return property;
    }

    if (include_parents) {
      for (const auto& entry : get_parent_registry_entries(registry_entry_)) {
        if (auto property = entry.find_string_property(key)) {
          return property;
        }
      }
    }

    return std::nullopt;
  }

  std::optional<int64_t> find_max_input_report_size(void) const {
    return find_int64_property(CFSTR(kIOHIDMaxInputReportSizeKey),
                               false);
  }

  std::optional<hid::vendor_id::value_t> find_vendor_id(void) const {
    if (auto value = find_int64_property(CFSTR(kIOHIDVendorIDKey),
                                         true)) {
      return hid::vendor_id::value_t(*value);
    }
    return std::nullopt;
  }

  std::optional<hid::product_id::value_t> find_product_id(void) const {
    if (auto value = find_int64_property(CFSTR(kIOHIDProductIDKey),
                                         true)) {
      return hid::product_id::value_t(*value);
    }
    return std::nullopt;
  }

  std::optional<hid::country_code::value_t> find_country_code(void) const {
    if (auto value = find_int64_property(CFSTR(kIOHIDCountryCodeKey),
                                         true)) {
      return hid::country_code::value_t(*value);
    }
    return std::nullopt;
  }

  std::optional<iokit_hid_location_id::value_t> find_location_id(void) const {
    if (auto value = find_int64_property(CFSTR(kIOHIDLocationIDKey),
                                         false)) {
      return iokit_hid_location_id::value_t(*value);
    }
    return std::nullopt;
  }

  std::optional<std::string> find_manufacturer(void) const {
    return find_string_property(CFSTR(kIOHIDManufacturerKey),
                                true);
  }

  std::optional<std::string> find_product(void) const {
    return find_string_property(CFSTR(kIOHIDProductKey),
                                true);
  }

  std::optional<std::string> find_serial_number(void) const {
    return find_string_property(CFSTR(kIOHIDSerialNumberKey),
                                true);
  }

  std::optional<std::string> find_transport(void) const {
    return find_string_property(CFSTR(kIOHIDTransportKey),
                                true);
  }

  std::optional<std::string> find_device_address(void) const {
    return find_string_property(CFSTR("DeviceAddress"),
                                true);
  }

  // Note:
  // Input Monitoring permission user approval is required since macOS Catalina (10.15).
  std::vector<cf::cf_ptr<IOHIDElementRef>> make_elements(void) {
    std::vector<cf::cf_ptr<IOHIDElementRef>> result;

    if (device_) {
      // Note:
      //
      // Some devices has duplicated entries of the same usage_page and usage.
      // For example, there are entries of Microsoft Designer Mouse:
      //
      //   * Microsoft Designer Mouse usage_page 1 usage 2
      //   * Microsoft Designer Mouse usage_page 1 usage 2
      //   * Microsoft Designer Mouse usage_page 1 usage 1
      //   * Microsoft Designer Mouse usage_page 1 usage 56
      //   * Microsoft Designer Mouse usage_page 1 usage 56
      //   * Microsoft Designer Mouse usage_page 1 usage 568
      //   * Microsoft Designer Mouse usage_page 12 usage 568
      //   * ...
      //
      // We should treat them as independent entries.
      // (== We should not combine them into one entry.)

      if (auto elements = IOHIDDeviceCopyMatchingElements(*device_, nullptr, kIOHIDOptionsTypeNone)) {
        for (CFIndex i = 0; i < CFArrayGetCount(elements); ++i) {
          auto e = cf::get_cf_array_value<IOHIDElementRef>(elements, i);
          if (e) {
            result.emplace_back(e);
          }
        }

        CFRelease(elements);
      }
    }

    return result;
  }

  cf::cf_ptr<IOHIDQueueRef> make_queue(CFIndex depth) const {
    cf::cf_ptr<IOHIDQueueRef> result;

    if (device_) {
      if (auto queue = IOHIDQueueCreate(kCFAllocatorDefault, *device_, depth, kIOHIDOptionsTypeNone)) {
        result = queue;

        CFRelease(queue);
      }
    }

    return result;
  }

private:
  std::vector<iokit_registry_entry> get_parent_registry_entries(iokit_registry_entry registry_entry) const {
    std::vector<iokit_registry_entry> result;

    // Only target registry entries with matching location id
    if (auto location_id = registry_entry.find_int64_property(CFSTR(kIOHIDLocationIDKey))) {
      auto parent_iterator = registry_entry.get_parent_iterator(kIOServicePlane);
      while (true) {
        auto next = parent_iterator.next();
        if (!next) {
          break;
        }

        pqrs::osx::iokit_registry_entry entry(next);
        if (auto parent_location_id = entry.find_int64_property(CFSTR(kIOHIDLocationIDKey))) {
          if (location_id == parent_location_id) {
            result.push_back(entry);
          }
        }

        for (const auto& e : get_parent_registry_entries(entry)) {
          result.push_back(e);
        }
      }
    }

    return result;
  }

  cf::cf_ptr<IOHIDDeviceRef> device_;
  iokit_object_ptr service_;
  iokit_registry_entry registry_entry_;
};
} // namespace osx
} // namespace pqrs
