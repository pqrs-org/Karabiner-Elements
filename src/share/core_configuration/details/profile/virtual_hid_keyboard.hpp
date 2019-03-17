#pragma once

#include "types.hpp"

namespace krbn {
namespace core_configuration {
namespace details {
class virtual_hid_keyboard final {
public:
  virtual_hid_keyboard(const nlohmann::json& json) : json_(json),
                                                     country_code_(0) {
    for (const auto& [key, value] : json.items()) {
      if (key == "country_code") {
        country_code_ = value.get<hid_country_code>();
      } else {
        // Allow unknown keys in order to be able to load
        // newer version of karabiner.json with older Karabiner-Elements.
      }
    }
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["country_code"] = country_code_;
    return j;
  }

  hid_country_code get_country_code(void) const {
    return country_code_;
  }

  void set_country_code(hid_country_code value) {
    country_code_ = value;
  }

  bool operator==(const virtual_hid_keyboard& other) const {
    return country_code_ == other.country_code_;
  }

private:
  nlohmann::json json_;
  hid_country_code country_code_;
};

inline void to_json(nlohmann::json& json, const virtual_hid_keyboard& virtual_hid_keyboard) {
  json = virtual_hid_keyboard.to_json();
}
} // namespace details
} // namespace core_configuration
} // namespace krbn
