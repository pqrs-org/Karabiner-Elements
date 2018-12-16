#pragma once

#include "descriptions.hpp"
#include "device_properties.hpp"
#include "json_utility.hpp"
#include "types.hpp"

namespace krbn {
namespace connected_devices {
namespace details {
class device final {
public:
  device(const descriptions& descriptions,
         const device_identifiers& identifiers,
         bool is_built_in_keyboard,
         bool is_built_in_trackpad) : descriptions_(descriptions),
                                      identifiers_(identifiers),
                                      is_built_in_keyboard_(is_built_in_keyboard),
                                      is_built_in_trackpad_(is_built_in_trackpad) {
  }

  device(const device_properties& device_properties) {
    descriptions_ = descriptions(device_properties);

    if (auto device_identifiers = device_properties.get_device_identifiers()) {
      identifiers_ = *device_identifiers;
    }

    is_built_in_keyboard_ = device_properties.get_is_built_in_keyboard().value_or(false);

    is_built_in_trackpad_ = device_properties.get_is_built_in_pointing_device().value_or(false);
  }

  static device make_from_json(const nlohmann::json& json) {
    return device(descriptions::make_from_json(
                      json_utility::find_copy(json, "descriptions", nlohmann::json())),
                  device_identifiers::make_from_json(
                      json_utility::find_copy(json, "identifiers", nlohmann::json())),
                  json_utility::find_optional<bool>(json, "is_built_in_keyboard").value_or(false),
                  json_utility::find_optional<bool>(json, "is_built_in_trackpad").value_or(false));
  }

  nlohmann::json to_json(void) const {
    return nlohmann::json({
        {"descriptions", descriptions_},
        {"identifiers", identifiers_},
        {"is_built_in_keyboard", is_built_in_keyboard_},
        {"is_built_in_trackpad", is_built_in_trackpad_},
    });
  }

  const descriptions& get_descriptions(void) const {
    return descriptions_;
  }

  const device_identifiers& get_identifiers(void) const {
    return identifiers_;
  }

  bool get_is_built_in_keyboard(void) const {
    return is_built_in_keyboard_;
  }

  bool get_is_built_in_trackpad(void) const {
    return is_built_in_trackpad_;
  }

  bool operator==(const device& other) const {
    return identifiers_ == other.identifiers_;
  }

private:
  descriptions descriptions_;
  device_identifiers identifiers_;
  bool is_built_in_keyboard_;
  bool is_built_in_trackpad_;
};

inline void to_json(nlohmann::json& json, const device& device) {
  json = device.to_json();
}
} // namespace details
} // namespace connected_devices
} // namespace krbn
