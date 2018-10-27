#pragma once

#include "descriptions.hpp"
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
  device(const nlohmann::json& json) : descriptions_(json_utility::find_copy(json, "descriptions", nlohmann::json())),
                                       identifiers_(json_utility::find_copy(json, "identifiers", nlohmann::json())),
                                       is_built_in_keyboard_(false),
                                       is_built_in_trackpad_(false) {
    if (auto v = json_utility::find_optional<bool>(json, "is_built_in_keyboard")) {
      is_built_in_keyboard_ = *v;
    }

    if (auto v = json_utility::find_optional<bool>(json, "is_built_in_trackpad")) {
      is_built_in_trackpad_ = *v;
    }
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
