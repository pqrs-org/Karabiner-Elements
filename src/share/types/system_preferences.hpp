#pragma once

#include "stream_utility.hpp"
#include <cstdint>

namespace krbn {
class system_preferences final {
public:
  system_preferences(void) : keyboard_fn_state_(false),
                             swipe_scroll_direction_(true),
                             keyboard_type_(40) {
  }

  bool get_keyboard_fn_state(void) const {
    return keyboard_fn_state_;
  }

  void set_keyboard_fn_state(bool value) {
    keyboard_fn_state_ = value;
  }

  bool get_swipe_scroll_direction(void) const {
    return swipe_scroll_direction_;
  }

  void set_swipe_scroll_direction(bool value) {
    swipe_scroll_direction_ = value;
  }

  uint8_t get_keyboard_type(void) const {
    return keyboard_type_;
  }

  void set_keyboard_type(uint8_t value) {
    keyboard_type_ = value;
  }

  bool operator==(const system_preferences& other) const {
    return keyboard_fn_state_ == other.keyboard_fn_state_ &&
           swipe_scroll_direction_ == other.swipe_scroll_direction_ &&
           keyboard_type_ == other.keyboard_type_;
  }

  bool operator!=(const system_preferences& other) const { return !(*this == other); }

private:
  bool keyboard_fn_state_;
  bool swipe_scroll_direction_;
  uint8_t keyboard_type_;
};

inline void to_json(nlohmann::json& j, const system_preferences& p) {
  j = nlohmann::json{
      {"keyboard_fn_state", p.get_keyboard_fn_state()},
      {"swipe_scroll_direction", p.get_swipe_scroll_direction()},
      {"keyboard_type", p.get_keyboard_type()},
  };
}

inline void from_json(const nlohmann::json& j, system_preferences& p) {
  try {
    p.set_keyboard_fn_state(j.at("keyboard_fn_state").get<bool>());
  } catch (...) {}

  try {
    p.set_swipe_scroll_direction(j.at("swipe_scroll_direction").get<bool>());
  } catch (...) {}

  try {
    p.set_keyboard_type(j.at("keyboard_type").get<uint8_t>());
  } catch (...) {}
}
} // namespace krbn
