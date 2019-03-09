#pragma once

#include "product_id.hpp"
#include "vendor_id.hpp"
#include <cstdint>
#include <pqrs/json.hpp>

namespace krbn {
class system_preferences_keyboard_type final {
public:
  system_preferences_keyboard_type(void) : vendor_id_(0),
                                           product_id_(0),
                                           country_code_(0),
                                           keyboard_type_(40) {
  }

  system_preferences_keyboard_type(pqrs::osx::iokit_hid_vendor_id vendor_id,
                                   pqrs::osx::iokit_hid_product_id product_id,
                                   pqrs::osx::iokit_hid_country_code country_code,
                                   pqrs::osx::iokit_keyboard_type keyboard_type) : vendor_id_(vendor_id),
                                                                                   product_id_(product_id),
                                                                                   country_code_(country_code),
                                                                                   keyboard_type_(keyboard_type) {
  }

  pqrs::osx::iokit_hid_vendor_id get_vendor_id(void) const {
    return vendor_id_;
  }

  void set_vendor_id(pqrs::osx::iokit_hid_vendor_id value) {
    vendor_id_ = value;
  }

  pqrs::osx::iokit_hid_product_id get_product_id(void) const {
    return product_id_;
  }

  void set_product_id(pqrs::osx::iokit_hid_product_id value) {
    product_id_ = value;
  }

  pqrs::osx::iokit_hid_country_code get_country_code(void) const {
    return country_code_;
  }

  void set_country_code(pqrs::osx::iokit_hid_country_code value) {
    country_code_ = value;
  }

  pqrs::osx::iokit_keyboard_type get_keyboard_type(void) const {
    return keyboard_type_;
  }

  void set_keyboard_type(pqrs::osx::iokit_keyboard_type value) {
    keyboard_type_ = value;
  }

  bool operator==(const system_preferences_keyboard_type& other) const {
    return vendor_id_ == other.vendor_id_ &&
           product_id_ == other.product_id_ &&
           country_code_ == other.country_code_ &&
           keyboard_type_ == other.keyboard_type_;
  }

  bool operator!=(const system_preferences_keyboard_type& other) const { return !(*this == other); }

private:
  pqrs::osx::iokit_hid_vendor_id vendor_id_;
  pqrs::osx::iokit_hid_product_id product_id_;
  pqrs::osx::iokit_hid_country_code country_code_;
  pqrs::osx::iokit_keyboard_type keyboard_type_;
};

inline void to_json(nlohmann::json& j, const system_preferences_keyboard_type& t) {
  j = nlohmann::json{
      {"vendor_id", t.get_vendor_id()},
      {"product_id", t.get_product_id()},
      {"country_code", t.get_country_code()},
      {"keyboard_type", t.get_keyboard_type()},
  };
}

inline void from_json(const nlohmann::json& j, system_preferences_keyboard_type& t) {
  if (!j.is_object()) {
    throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", j.dump()));
  }

  for (const auto& [key, value] : j.items()) {
    if (key == "vendor_id") {
      t.set_vendor_id(value.get<pqrs::osx::iokit_hid_vendor_id>());

    } else if (key == "product_id") {
      t.set_product_id(value.get<pqrs::osx::iokit_hid_product_id>());

    } else if (key == "country_code") {
      t.set_country_code(value.get<pqrs::osx::iokit_hid_country_code>());

    } else if (key == "keyboard_type") {
      t.set_keyboard_type(value.get<pqrs::osx::iokit_keyboard_type>());

    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, j.dump()));
    }
  }
}
} // namespace krbn
