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

  [[nodiscard]] const connected_device* find_device(const device_identifiers& identifiers) const {
    auto it = std::find_if(std::begin(devices_),
                           std::end(devices_),
                           [&](const auto& d) {
                             return d.properties->get_device_identifiers() == identifiers;
                           });
    if (it != std::end(devices_)) {
      return &*it;
    }

    return nullptr;
  }

  void push_back_device(
      pqrs::not_null_shared_ptr_t<device_properties> properties,
      const connected_device_capabilities& capabilities = connected_device_capabilities()) {
    if (find_device(properties->get_device_identifiers())) {
      return;
    }

    auto device = connected_device{
        .properties = properties,
        .capabilities = capabilities,
    };
    auto position = std::lower_bound(
        devices_.begin(),
        devices_.end(),
        device,
        [](const auto& a, const auto& b) {
          return a.properties->compare(*b.properties);
        });
    devices_.insert(position, std::move(device));
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

    value.push_back_device(device_properties::make_device_properties(j),
                           capabilities);
  }
}

inline void to_json(nlohmann::json& json, const connected_devices& value) {
  json = nlohmann::json::array();

  for (const auto& device : value.get_devices()) {
    nlohmann::json device_json = *device.properties;
    if (!device.capabilities.empty()) {
      device_json["connected_device_capabilities"] = device.capabilities.to_json();
    }
    json.push_back(std::move(device_json));
  }
}
} // namespace krbn
