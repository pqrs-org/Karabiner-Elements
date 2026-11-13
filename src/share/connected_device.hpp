#pragma once

#include "connected_device_capabilities.hpp"
#include "device_properties.hpp"
#include "json_utility.hpp"
#include <algorithm>
#include <nlohmann/json.hpp>
#include <pqrs/gsl.hpp>
#include <string>
#include <utility>
#include <vector>

namespace krbn {
struct connected_device final {
  struct device_identifier_value_for_ui final {
    location_id location_id{0};
    std::string device_address;
    std::string serial_number;
    std::string transport;

    bool operator==(const device_identifier_value_for_ui&) const = default;

    [[nodiscard]] bool empty() const {
      return location_id == krbn::location_id(0) &&
             device_address.empty() &&
             serial_number.empty() &&
             transport.empty();
    }
  };

  // connected_device represents a group of devices that share device_identifiers.
  // This class keeps identifier-related values for each physical device together
  // for displaying the devices in UI clients. These values are collected
  // regardless of additional_device_identifier_fields and are not used to form
  // the group.
  class device_identifier_values_for_ui final {
  public:
    void add(const device_properties& value) {
      add(device_identifier_value_for_ui{
          .location_id = value.get_location_id(),
          .device_address = value.get_device_address(),
          .serial_number = value.get_serial_number(),
          .transport = value.get_transport(),
      });
    }

    void add(device_identifier_value_for_ui value) {
      if (!value.empty() &&
          std::find(std::begin(values_),
                    std::end(values_),
                    value) == std::end(values_)) {
        values_.push_back(std::move(value));

        // Keep the order stable using the field order shown in DevicesView.
        std::sort(std::begin(values_),
                  std::end(values_),
                  [](const auto& a, const auto& b) {
                    if (a.device_address != b.device_address) {
                      return a.device_address < b.device_address;
                    }
                    if (a.location_id != b.location_id) {
                      return type_safe::get(a.location_id) < type_safe::get(b.location_id);
                    }
                    if (a.serial_number != b.serial_number) {
                      return a.serial_number < b.serial_number;
                    }
                    return a.transport < b.transport;
                  });
      }
    }

    [[nodiscard]] const std::vector<device_identifier_value_for_ui>& get_values() const {
      return values_;
    }

    void clear() {
      values_.clear();
    }

  private:
    std::vector<device_identifier_value_for_ui> values_;
  };

  connected_device(pqrs::not_null_shared_ptr_t<device_properties> properties,
                   const connected_device_capabilities& capabilities)
      : properties(std::move(properties)),
        capabilities(capabilities) {
    device_identifier_values_for_ui_.add(*this->properties);
  }

  pqrs::not_null_shared_ptr_t<device_properties> properties;
  connected_device_capabilities capabilities;

  [[nodiscard]] device_identifier_values_for_ui& get_device_identifier_values_for_ui() {
    return device_identifier_values_for_ui_;
  }

  [[nodiscard]] const device_identifier_values_for_ui& get_device_identifier_values_for_ui() const {
    return device_identifier_values_for_ui_;
  }

private:
  device_identifier_values_for_ui device_identifier_values_for_ui_;
};

inline void from_json(const nlohmann::json& json,
                      connected_device::device_identifier_values_for_ui& value) {
  pqrs::json::requires_array(json, "`device_identifier_values_for_ui`");

  value.clear();

  for (const auto& entry : json) {
    pqrs::json::requires_object(entry, "`device_identifier_values_for_ui` entry");
    connected_device::device_identifier_value_for_ui identifier_value;

    for (const auto& [key, v] : entry.items()) {
      if (key == "location_id") {
        pqrs::json::requires_number(v, "`device_identifier_values_for_ui.location_id`");
        identifier_value.location_id = location_id(v.get<uint64_t>());

      } else if (key == "device_address") {
        pqrs::json::requires_string(v, "`device_identifier_values_for_ui.device_address`");
        identifier_value.device_address = v.get<std::string>();

      } else if (key == "serial_number") {
        pqrs::json::requires_string(v, "`device_identifier_values_for_ui.serial_number`");
        identifier_value.serial_number = v.get<std::string>();

      } else if (key == "transport") {
        pqrs::json::requires_string(v, "`device_identifier_values_for_ui.transport`");
        identifier_value.transport = v.get<std::string>();

      } else {
        // Allow unknown key
      }
    }

    value.add(std::move(identifier_value));
  }
}

inline void to_json(nlohmann::json& json,
                    const connected_device::device_identifier_values_for_ui& value) {
  json = nlohmann::json::array();

  for (const auto& v : value.get_values()) {
    nlohmann::json entry;

    if (v.location_id != location_id(0)) {
      entry["location_id"] = type_safe::get(v.location_id);
    }
    if (!v.device_address.empty()) {
      entry["device_address"] = v.device_address;
    }
    if (!v.serial_number.empty()) {
      entry["serial_number"] = v.serial_number;
    }
    if (!v.transport.empty()) {
      entry["transport"] = v.transport;
    }

    json.push_back(std::move(entry));
  }
}
} // namespace krbn
