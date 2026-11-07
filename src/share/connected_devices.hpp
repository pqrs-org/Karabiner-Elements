#pragma once

#include "connected_device_capabilities.hpp"
#include "device_properties.hpp"
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
  [[nodiscard]] const std::vector<pqrs::not_null_shared_ptr_t<device_properties>>& get_devices() const {
    return devices_;
  }

  [[nodiscard]] const connected_device_capabilities& get_capabilities(size_t index) const {
    return capabilities_.at(index);
  }

  [[nodiscard]] std::shared_ptr<device_properties> find_device(const device_identifiers& identifiers) const {
    auto it = std::find_if(std::begin(devices_),
                           std::end(devices_),
                           [&](const auto& d) {
                             return d->get_device_identifiers() == identifiers;
                           });
    if (it != std::end(devices_)) {
      return *it;
    }

    return nullptr;
  }

  void push_back_device(
      pqrs::not_null_shared_ptr_t<device_properties> device,
      const connected_device_capabilities& capabilities = connected_device_capabilities()) {
    if (find_device(device->get_device_identifiers())) {
      return;
    }

    auto position = std::lower_bound(
        devices_.begin(),
        devices_.end(),
        device,
        [](const auto& a, const auto& b) {
          return a->compare(*b);
        });
    auto index = static_cast<size_t>(std::distance(devices_.begin(), position));
    devices_.insert(position, device);
    capabilities_.insert(capabilities_.begin() + index, capabilities);
  }

  void clear() {
    devices_.clear();
    capabilities_.clear();
  }

private:
  std::vector<pqrs::not_null_shared_ptr_t<device_properties>> devices_;
  std::vector<connected_device_capabilities> capabilities_;
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

  for (size_t i = 0; i < value.get_devices().size(); ++i) {
    nlohmann::json device_json = *value.get_devices()[i];
    const auto& capabilities = value.get_capabilities(i);
    if (!capabilities.empty()) {
      device_json["connected_device_capabilities"] = capabilities.to_json();
    }
    json.push_back(std::move(device_json));
  }
}
} // namespace krbn
