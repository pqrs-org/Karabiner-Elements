#pragma once

// pqrs::osx::iokit_hid_device v5.1.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <optional>
#include <pqrs/cf/array.hpp>
#include <pqrs/cf/cf_ptr.hpp>
#include <pqrs/cf/string.hpp>
#include <pqrs/hid.hpp>
#include <pqrs/osx/iokit_object_ptr.hpp>
#include <pqrs/osx/iokit_registry_entry.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace pqrs::osx {
class iokit_hid_device final {
public:
  iokit_hid_device(IOHIDDeviceRef device)
      : device_(device),
        service_(device ? IOHIDDeviceGetService(device) : IO_OBJECT_NULL),
        registry_entry_(service_) {
  }

  ~iokit_hid_device() = default;

  [[nodiscard]] cf::cf_ptr<IOHIDDeviceRef> get_device() const noexcept {
    return device_;
  }

  [[nodiscard]] iokit_object_ptr get_service() const noexcept {
    return service_;
  }

  [[nodiscard]] iokit_registry_entry get_registry_entry() const {
    return registry_entry_;
  }

  // Note:
  // Input Monitoring permission user approval is required since macOS Catalina (10.15).
  [[nodiscard]] bool conforms_to(hid::usage_page::value_t usage_page, hid::usage::value_t usage) const noexcept {
    if (device_) {
      return IOHIDDeviceConformsTo(*device_,
                                   type_safe::get(usage_page),
                                   type_safe::get(usage));
    }

    return false;
  }

  [[nodiscard]] std::optional<int64_t> find_int64_property(CFStringRef key, bool include_parents = false) const {
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

  [[nodiscard]] std::optional<std::string> find_string_property(CFStringRef key, bool include_parents = false) const {
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

  [[nodiscard]] std::optional<int64_t> find_max_input_report_size() const {
    return find_int64_property(CFSTR(kIOHIDMaxInputReportSizeKey),
                               false);
  }

  [[nodiscard]] std::optional<hid::vendor_id::value_t> find_vendor_id() const {
    if (auto value = find_int64_property(CFSTR(kIOHIDVendorIDKey),
                                         true)) {
      return hid::vendor_id::value_t(*value);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<hid::product_id::value_t> find_product_id() const {
    if (auto value = find_int64_property(CFSTR(kIOHIDProductIDKey),
                                         true)) {
      return hid::product_id::value_t(*value);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<hid::country_code::value_t> find_country_code() const {
    if (auto value = find_int64_property(CFSTR(kIOHIDCountryCodeKey),
                                         true)) {
      return hid::country_code::value_t(*value);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<iokit_hid_location_id::value_t> find_location_id() const {
    if (auto value = find_int64_property(CFSTR(kIOHIDLocationIDKey),
                                         false)) {
      return iokit_hid_location_id::value_t(*value);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<hid::manufacturer_string::value_t> find_manufacturer() const {
    if (auto value = find_string_property(CFSTR(kIOHIDManufacturerKey),
                                          true)) {
      return hid::manufacturer_string::value_t(*value);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<hid::product_string::value_t> find_product() const {
    if (auto value = find_string_property(CFSTR(kIOHIDProductKey),
                                          true)) {
      return hid::product_string::value_t(*value);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<std::string> find_serial_number() const {
    return find_string_property(CFSTR(kIOHIDSerialNumberKey),
                                true);
  }

  [[nodiscard]] std::optional<std::string> find_transport() const {
    return find_string_property(CFSTR(kIOHIDTransportKey),
                                true);
  }

  [[nodiscard]] std::optional<std::string> find_device_address() const {
    return find_string_property(CFSTR("DeviceAddress"),
                                true);
  }

  // Note:
  // Input Monitoring permission user approval is required since macOS Catalina (10.15).
  [[nodiscard]] std::vector<cf::cf_ptr<IOHIDElementRef>> make_elements() {
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

      if (auto elements = cf::adopt_cf_ptr(IOHIDDeviceCopyMatchingElements(*device_, nullptr, kIOHIDOptionsTypeNone))) {
        for (CFIndex i = 0; i < CFArrayGetCount(*elements); ++i) {
          auto e = cf::get_cf_array_value<IOHIDElementRef>(*elements, i);
          if (e) {
            result.emplace_back(e);
          }
        }
      }
    }

    return result;
  }

  [[nodiscard]] cf::cf_ptr<IOHIDQueueRef> make_queue(CFIndex depth) const noexcept {
    cf::cf_ptr<IOHIDQueueRef> result;

    if (device_) {
      result = cf::adopt_cf_ptr(IOHIDQueueCreate(kCFAllocatorDefault, *device_, depth, kIOHIDOptionsTypeNone));
    }

    return result;
  }

private:
  [[nodiscard]] std::vector<iokit_registry_entry> get_parent_registry_entries(iokit_registry_entry registry_entry) const {
    std::vector<iokit_registry_entry> result;

    // Only target registry entries with matching location id
    if (auto location_id = registry_entry.find_int64_property(CFSTR(kIOHIDLocationIDKey))) {
      auto parent_iterator = registry_entry.get_parent_iterator(kIOServicePlane);
      while (auto next = parent_iterator.next()) {
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
} // namespace pqrs::osx
