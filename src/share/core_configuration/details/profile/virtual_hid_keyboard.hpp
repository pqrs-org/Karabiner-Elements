#pragma once

#include "types.hpp"
#include <pqrs/hid.hpp>

namespace krbn {
namespace core_configuration {
namespace details {
class virtual_hid_keyboard final {
public:
  virtual_hid_keyboard(void) : json_(nlohmann::json::object()),
                               country_code_(0),
                               mouse_key_xy_scale_(100),
                               indicate_sticky_modifier_keys_state_(true) {
  }

  const nlohmann::json& get_json(void) const {
    return json_;
  }

  void set_json(const nlohmann::json& value) {
    json_ = value;
  }

  pqrs::hid::country_code::value_t get_country_code(void) const {
    return country_code_;
  }

  void set_country_code(pqrs::hid::country_code::value_t value) {
    country_code_ = value;
  }

  int get_mouse_key_xy_scale(void) const {
    return mouse_key_xy_scale_;
  }

  void set_mouse_key_xy_scale(int value) {
    if (value < 0) {
      value = 0;
    }
    mouse_key_xy_scale_ = value;
  }

  bool get_indicate_sticky_modifier_keys_state(void) const {
    return indicate_sticky_modifier_keys_state_;
  }

  void set_indicate_sticky_modifier_keys_state(bool value) {
    indicate_sticky_modifier_keys_state_ = value;
  }

  bool operator==(const virtual_hid_keyboard& other) const {
    // Skip `json_`.
    return country_code_ == other.country_code_ &&
           mouse_key_xy_scale_ == other.mouse_key_xy_scale_ &&
           indicate_sticky_modifier_keys_state_ == other.indicate_sticky_modifier_keys_state_;
  }

private:
  nlohmann::json json_;
  pqrs::hid::country_code::value_t country_code_;
  int mouse_key_xy_scale_;
  bool indicate_sticky_modifier_keys_state_;
};

inline void to_json(nlohmann::json& json, const virtual_hid_keyboard& value) {
  json = value.get_json();
  json["country_code"] = value.get_country_code();
  json["mouse_key_xy_scale"] = value.get_mouse_key_xy_scale();
  json["indicate_sticky_modifier_keys_state"] = value.get_indicate_sticky_modifier_keys_state();
}

inline void from_json(const nlohmann::json& json, virtual_hid_keyboard& value) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [k, v] : json.items()) {
    if (k == "country_code") {
      value.set_country_code(v.get<pqrs::hid::country_code::value_t>());

    } else if (k == "mouse_key_xy_scale") {
      pqrs::json::requires_number(v, "`" + k + "`");

      value.set_mouse_key_xy_scale(v.get<int>());

    } else if (k == "indicate_sticky_modifier_keys_state") {
      pqrs::json::requires_boolean(v, "`" + k + "`");

      value.set_indicate_sticky_modifier_keys_state(v.get<int>());

    } else {
      // Allow unknown keys in order to be able to load
      // newer version of karabiner.json with older Karabiner-Elements.
    }
  }

  value.set_json(json);
}
} // namespace details
} // namespace core_configuration
} // namespace krbn

namespace std {
template <>
struct hash<krbn::core_configuration::details::virtual_hid_keyboard> final {
  std::size_t operator()(const krbn::core_configuration::details::virtual_hid_keyboard& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_country_code());
    pqrs::hash::combine(h, value.get_mouse_key_xy_scale());
    pqrs::hash::combine(h, value.get_indicate_sticky_modifier_keys_state());

    return h;
  }
};
} // namespace std
