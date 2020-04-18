#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <pqrs/hid.hpp>
#include <pqrs/hid/extra/nlohmann_json.hpp>

namespace krbn {
class device_identifiers final {
public:
  device_identifiers(void) : vendor_id_(pqrs::hid::vendor_id::value_t(0)),
                             product_id_(pqrs::hid::product_id::value_t(0)),
                             is_keyboard_(false),
                             is_pointing_device_(false) {
  }

  device_identifiers(pqrs::hid::vendor_id::value_t vendor_id,
                     pqrs::hid::product_id::value_t product_id,
                     bool is_keyboard,
                     bool is_pointing_device) : vendor_id_(vendor_id),
                                                product_id_(product_id),
                                                is_keyboard_(is_keyboard),
                                                is_pointing_device_(is_pointing_device) {
  }

  const nlohmann::json& get_json(void) const {
    return json_;
  }

  void set_json(const nlohmann::json& value) {
    json_ = value;
  }

  pqrs::hid::vendor_id::value_t get_vendor_id(void) const {
    return vendor_id_;
  }

  void set_vendor_id(pqrs::hid::vendor_id::value_t value) {
    vendor_id_ = value;
  }

  pqrs::hid::product_id::value_t get_product_id(void) const {
    return product_id_;
  }

  void set_product_id(pqrs::hid::product_id::value_t value) {
    product_id_ = value;
  }

  bool get_is_keyboard(void) const {
    return is_keyboard_;
  }

  void set_is_keyboard(bool value) {
    is_keyboard_ = value;
  }

  bool get_is_pointing_device(void) const {
    return is_pointing_device_;
  }

  void set_is_pointing_device(bool value) {
    is_pointing_device_ = value;
  }

  bool is_apple(void) const {
    return vendor_id_ == pqrs::hid::vendor_id::value_t(0x05ac) ||
           vendor_id_ == pqrs::hid::vendor_id::value_t(0x004c);
  }

  bool operator==(const device_identifiers& other) const {
    return vendor_id_ == other.vendor_id_ &&
           product_id_ == other.product_id_ &&
           is_keyboard_ == other.is_keyboard_ &&
           is_pointing_device_ == other.is_pointing_device_;
  }

private:
  nlohmann::json json_;
  pqrs::hid::vendor_id::value_t vendor_id_;
  pqrs::hid::product_id::value_t product_id_;
  bool is_keyboard_;
  bool is_pointing_device_;
};

inline void to_json(nlohmann::json& json, const device_identifiers& value) {
  json = value.get_json();
  json["vendor_id"] = type_safe::get(value.get_vendor_id());
  json["product_id"] = type_safe::get(value.get_product_id());
  json["is_keyboard"] = value.get_is_keyboard();
  json["is_pointing_device"] = value.get_is_pointing_device();
}

inline void from_json(const nlohmann::json& json, device_identifiers& value) {
  if (!json.is_object()) {
    throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
  }

  for (const auto& [k, v] : json.items()) {
    if (k == "vendor_id") {
      if (!v.is_number()) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", k, v.dump()));
      }

      value.set_vendor_id(v.get<pqrs::hid::vendor_id::value_t>());

    } else if (k == "product_id") {
      if (!v.is_number()) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", k, v.dump()));
      }

      value.set_product_id(v.get<pqrs::hid::product_id::value_t>());

    } else if (k == "is_keyboard") {
      if (!v.is_boolean()) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be boolean, but is `{1}`", k, v.dump()));
      }

      value.set_is_keyboard(v.get<bool>());

    } else if (k == "is_pointing_device") {
      if (!v.is_boolean()) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be boolean, but is `{1}`", k, v.dump()));
      }

      value.set_is_pointing_device(v.get<bool>());

    } else {
      // Allow unknown key
    }
  }

  value.set_json(json);
}
} // namespace krbn
