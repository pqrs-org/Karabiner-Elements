#pragma once

#include "connected_device.hpp"
#include "json_utility.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <algorithm>
#include <fstream>
#include <gsl/gsl>
#include <nlohmann/json.hpp>
#include <pqrs/filesystem.hpp>

namespace krbn {
class connected_devices final {
public:
  [[nodiscard]] const std::vector<connected_device>& get_devices() const {
    return devices_;
  }

  [[nodiscard]] bool contains(const device_identifiers& identifiers) const {
    return std::any_of(std::begin(devices_),
                       std::end(devices_),
                       [&](const auto& device) {
                         return device.properties->get_device_identifiers() == identifiers;
                       });
  }

  connected_device& push_back_device(
      pqrs::not_null_shared_ptr_t<device_properties> properties,
      const connected_device_capabilities& capabilities = connected_device_capabilities()) {
    for (auto& device : devices_) {
      if (device.properties->get_device_identifiers() == properties->get_device_identifiers()) {
        device.get_device_identifier_values_for_ui().add(*properties);
        return device;
      }
    }

    auto device = connected_device(properties,
                                   capabilities);
    auto position = std::lower_bound(
        devices_.begin(),
        devices_.end(),
        device,
        [](const auto& a, const auto& b) {
          return a.properties->compare(*b.properties);
        });
    return *devices_.insert(position,
                            std::move(device));
  }

  void clear() {
    devices_.clear();
  }

private:
  std::vector<connected_device> devices_;
};

inline void from_json(const nlohmann::json& json, connected_devices& value) {
  pqrs::json::requires_array(json, "`connected_devices`");

  for (const auto& j : json) {
    connected_device_capabilities capabilities;
    if (auto it = j.find("connected_device_capabilities");
        it != j.end()) {
      capabilities = connected_device_capabilities::make_connected_device_capabilities(*it);
    }

    auto& device = value.push_back_device(device_properties::make_device_properties(j),
                                          capabilities);

    if (auto it = j.find("device_identifier_values_for_ui");
        it != j.end()) {
      it->get_to(device.get_device_identifier_values_for_ui());
    }
  }
}

inline void to_json(nlohmann::json& json, const connected_devices& value) {
  json = nlohmann::json::array();

  for (const auto& device : value.get_devices()) {
    nlohmann::json device_json = *device.properties;

    auto capabilities_json = device.capabilities.to_json();
    if (!capabilities_json.empty()) {
      device_json["connected_device_capabilities"] = std::move(capabilities_json);
    }

    const auto& values = device.get_device_identifier_values_for_ui().get_values();
    if (!values.empty()) {
      device_json["device_identifier_values_for_ui"] = device.get_device_identifier_values_for_ui();
    }

    json.push_back(std::move(device_json));
  }
}
} // namespace krbn
