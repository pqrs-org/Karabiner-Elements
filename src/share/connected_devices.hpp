#pragma once

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
  connected_devices(void) {
  }

  const std::vector<pqrs::not_null_shared_ptr_t<device_properties>>& get_devices(void) const {
    return devices_;
  }

  std::shared_ptr<device_properties> find_device(const device_identifiers& identifiers) const {
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

  void push_back_device(pqrs::not_null_shared_ptr_t<device_properties> device) {
    if (find_device(device->get_device_identifiers())) {
      return;
    }

    devices_.push_back(device);

    std::sort(devices_.begin(),
              devices_.end(),
              [](auto& a, auto& b) {
                return a->compare(*b);
              });
  }

  void clear(void) {
    devices_.clear();
  }

private:
  std::vector<pqrs::not_null_shared_ptr_t<device_properties>> devices_;
};

inline void from_json(const nlohmann::json& json, connected_devices& value) {
  pqrs::json::requires_array(json, "`connected_devices`");

  for (const auto& j : json) {
    value.push_back_device(device_properties::make_device_properties(j));
  }
}

inline void to_json(nlohmann::json& json, const connected_devices& value) {
  json = nlohmann::json::array();

  for (const auto& d : value.get_devices()) {
    json.push_back(*d);
  }
}
} // namespace krbn
