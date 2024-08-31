#pragma once

#include "../../configuration_json_helper.hpp"
#include "types.hpp"
#include <pqrs/hid.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace krbn {
namespace core_configuration {
namespace details {
class virtual_hid_keyboard final {
public:
  virtual_hid_keyboard(const virtual_hid_keyboard&) = delete;

  virtual_hid_keyboard(void)
      : virtual_hid_keyboard(nlohmann::json::object(),
                             krbn::core_configuration::error_handling::loose) {
  }

  virtual_hid_keyboard(const nlohmann::json& json,
                       error_handling error_handling)
      : json_(json) {
    helper_values_.push_back_value<std::string>("keyboard_type_v2",
                                                keyboard_type_v2_,
                                                "");

    helper_values_.push_back_value<int>("mouse_key_xy_scale",
                                        mouse_key_xy_scale_,
                                        100);

    helper_values_.push_back_value<bool>("indicate_sticky_modifier_keys_state",
                                         indicate_sticky_modifier_keys_state_,
                                         true);

    pqrs::json::requires_object(json, "json");

    helper_values_.update_value(json, error_handling);
  }

  nlohmann::json to_json(void) const {
    auto j = json_;

    helper_values_.update_json(j);

    return j;
  }

  const std::string& get_keyboard_type_v2(void) const {
    return keyboard_type_v2_;
  }

  void set_keyboard_type_v2(const std::string& value) {
    keyboard_type_v2_ = value;
  }

  pqrs::osx::iokit_keyboard_type::value_t get_iokit_keyboard_type(void) const {
    if (keyboard_type_v2_ == "iso") {
      return pqrs::osx::iokit_keyboard_type::iso;
    } else if (keyboard_type_v2_ == "jis") {
      return pqrs::osx::iokit_keyboard_type::jis;
    } else {
      return pqrs::osx::iokit_keyboard_type::ansi;
    }
  }

  const int& get_mouse_key_xy_scale(void) const {
    return mouse_key_xy_scale_;
  }

  void set_mouse_key_xy_scale(int value) {
    if (value < 0) {
      value = 0;
    }
    mouse_key_xy_scale_ = value;
  }

  const bool& get_indicate_sticky_modifier_keys_state(void) const {
    return indicate_sticky_modifier_keys_state_;
  }

  void set_indicate_sticky_modifier_keys_state(bool value) {
    indicate_sticky_modifier_keys_state_ = value;
  }

  bool operator==(const virtual_hid_keyboard& other) const {
    // Skip `json_`.
    return keyboard_type_v2_ == other.keyboard_type_v2_ &&
           mouse_key_xy_scale_ == other.mouse_key_xy_scale_ &&
           indicate_sticky_modifier_keys_state_ == other.indicate_sticky_modifier_keys_state_;
  }

private:
  nlohmann::json json_;
  std::string keyboard_type_v2_;
  int mouse_key_xy_scale_;
  bool indicate_sticky_modifier_keys_state_;
  configuration_json_helper::helper_values helper_values_;
};
} // namespace details
} // namespace core_configuration
} // namespace krbn

namespace std {
template <>
struct hash<krbn::core_configuration::details::virtual_hid_keyboard> final {
  std::size_t operator()(const krbn::core_configuration::details::virtual_hid_keyboard& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_keyboard_type_v2());
    pqrs::hash::combine(h, value.get_mouse_key_xy_scale());
    pqrs::hash::combine(h, value.get_indicate_sticky_modifier_keys_state());

    return h;
  }
};
} // namespace std
