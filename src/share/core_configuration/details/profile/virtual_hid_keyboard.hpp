#pragma once

#include "types.hpp"

namespace krbn {
namespace core_configuration {
namespace details {
class virtual_hid_keyboard final {
public:
  virtual_hid_keyboard(void) : virtual_hid_keyboard(nlohmann::json::object()) {
  }

  virtual_hid_keyboard(const nlohmann::json& json) : json_(json),
                                                     country_code_(0),
                                                     mouse_key_xy_scale_(100) {
    if (!json.is_object()) {
      throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
    }

    for (const auto& [key, value] : json.items()) {
      if (key == "country_code") {
        country_code_ = value.get<hid_country_code>();

      } else if (key == "mouse_key_xy_scale") {
        mouse_key_xy_scale_ = value.get<int>();

      } else {
        // Allow unknown keys in order to be able to load
        // newer version of karabiner.json with older Karabiner-Elements.
      }
    }
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["country_code"] = country_code_;
    j["mouse_key_xy_scale"] = mouse_key_xy_scale_;
    return j;
  }

  hid_country_code get_country_code(void) const {
    return country_code_;
  }

  void set_country_code(hid_country_code value) {
    country_code_ = value;
  }

  double get_mouse_key_xy_scale(void) const {
    return static_cast<double>(mouse_key_xy_scale_) / 100.0;
  }

  void set_mouse_key_xy_scale(int value) {
    if (value < 0) {
      value = 0;
    }
    mouse_key_xy_scale_ = value;
  }

  bool operator==(const virtual_hid_keyboard& other) const {
    return country_code_ == other.country_code_;
  }

private:
  nlohmann::json json_;
  hid_country_code country_code_;
  int mouse_key_xy_scale_;
};

inline void to_json(nlohmann::json& json, const virtual_hid_keyboard& virtual_hid_keyboard) {
  json = virtual_hid_keyboard.to_json();
}
} // namespace details
} // namespace core_configuration
} // namespace krbn
