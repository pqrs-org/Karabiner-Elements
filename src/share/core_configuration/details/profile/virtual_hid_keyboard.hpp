#pragma once

#include "json_utility.hpp"

namespace krbn {
namespace core_configuration {
namespace details {
class virtual_hid_keyboard final {
public:
  virtual_hid_keyboard(const nlohmann::json& json) : json_(json),
                                                     country_code_(0) {
    if (auto v = json_utility::find_optional<uint8_t>(json, "country_code")) {
      country_code_ = *v;
    }
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["country_code"] = country_code_;
    return j;
  }

  uint8_t get_country_code(void) const {
    return country_code_;
  }
  void set_country_code(uint8_t value) {
    country_code_ = value;
  }

  bool operator==(const virtual_hid_keyboard& other) const {
    return country_code_ == other.country_code_;
  }

private:
  nlohmann::json json_;
  uint8_t country_code_;
};

inline void to_json(nlohmann::json& json, const virtual_hid_keyboard& virtual_hid_keyboard) {
  json = virtual_hid_keyboard.to_json();
}
} // namespace details
} // namespace core_configuration
} // namespace krbn
