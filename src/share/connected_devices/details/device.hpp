#pragma once

#include "descriptions.hpp"
#include "device_properties.hpp"
#include "types.hpp"

namespace krbn {
namespace connected_devices {
namespace details {
class device final {
public:
  device(const device&) = delete;

  device(const descriptions& descriptions,
         const device_identifiers& identifiers,
         bool is_built_in_keyboard,
         bool is_built_in_trackpad,
         bool is_built_in_touch_bar) : descriptions_(descriptions),
                                       identifiers_(identifiers),
                                       is_built_in_keyboard_(is_built_in_keyboard),
                                       is_built_in_trackpad_(is_built_in_trackpad),
                                       is_built_in_touch_bar_(is_built_in_touch_bar) {
  }

  device(const device_properties& device_properties) {
    descriptions_ = descriptions(device_properties);

    identifiers_ = device_properties.get_device_identifiers();

    is_built_in_keyboard_ = device_properties.get_is_built_in_keyboard();

    is_built_in_trackpad_ = device_properties.get_is_built_in_pointing_device();

    is_built_in_touch_bar_ = device_properties.get_is_built_in_touch_bar();
  }

  device(const nlohmann::json& json) {
    if (auto j = pqrs::json::find_json(json, "descriptions")) {
      descriptions_ = descriptions::make_from_json(j->value());
    }

    if (auto j = pqrs::json::find_json(json, "identifiers")) {
      identifiers_ = j->value().get<device_identifiers>();
    }

    is_built_in_keyboard_ = pqrs::json::find<bool>(json, "is_built_in_keyboard").value_or(false);

    is_built_in_trackpad_ = pqrs::json::find<bool>(json, "is_built_in_trackpad").value_or(false);

    is_built_in_touch_bar_ = pqrs::json::find<bool>(json, "is_built_in_touch_bar").value_or(false);
  }

  nlohmann::json to_json(void) const {
    return nlohmann::json({
        {"descriptions", descriptions_},
        {"identifiers", identifiers_},
        {"is_built_in_keyboard", is_built_in_keyboard_},
        {"is_built_in_trackpad", is_built_in_trackpad_},
        {"is_built_in_touch_bar", is_built_in_touch_bar_},
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

  bool get_is_built_in_touch_bar(void) const {
    return is_built_in_touch_bar_;
  }

  bool is_apple(void) const {
    if (identifiers_.get_is_virtual_device()) {
      return false;
    }

    return identifiers_.get_vendor_id() == pqrs::hid::vendor_id::value_t(0x05ac) ||
           identifiers_.get_vendor_id() == pqrs::hid::vendor_id::value_t(0x004c) ||
           descriptions_.get_product() == pqrs::hid::product_string::value_t("Apple Internal Keyboard / Trackpad");
  }

private:
  descriptions descriptions_;
  device_identifiers identifiers_;
  bool is_built_in_keyboard_;
  bool is_built_in_trackpad_;
  bool is_built_in_touch_bar_;
};

inline void to_json(nlohmann::json& json, const device& device) {
  json = device.to_json();
}
} // namespace details
} // namespace connected_devices
} // namespace krbn
