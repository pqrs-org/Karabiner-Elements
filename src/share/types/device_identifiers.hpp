#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <pqrs/hid.hpp>
#include <pqrs/hid/extra/nlohmann_json.hpp>

namespace krbn {
class device_identifiers final {
public:
  device_identifiers(void)
      : vendor_id_(pqrs::hid::vendor_id::value_t(0)),
        product_id_(pqrs::hid::product_id::value_t(0)),
        is_keyboard_(false),
        is_pointing_device_(false),
        is_game_pad_(false),
        device_address_("") {
  }

  device_identifiers(pqrs::hid::vendor_id::value_t vendor_id,
                     pqrs::hid::product_id::value_t product_id,
                     bool is_keyboard,
                     bool is_pointing_device,
                     bool is_game_pad,
                     std::string device_address)
      : vendor_id_(vendor_id),
        product_id_(product_id),
        is_keyboard_(is_keyboard),
        is_pointing_device_(is_pointing_device),
        is_game_pad_(is_game_pad) {
    // Some bluetooth devices do not have a vendor_id or product_id.
    // Such devices use device_address to distinguish between devices.
    // The device_address will be changed when the hardware is replaced,
    // so we use device_address only when vendor_id and product_id are zero.
    if (vendor_id == pqrs::hid::vendor_id::value_t(0) &&
        product_id == pqrs::hid::product_id::value_t(0)) {
      device_address_ = device_address;
    }
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

  bool get_is_game_pad(void) const {
    return is_game_pad_;
  }

  void set_is_game_pad(bool value) {
    is_game_pad_ = value;
  }

  const std::string& get_device_address(void) const {
    return device_address_;
  }

  void set_device_address(std::string value) {
    device_address_ = value;
  }

  bool is_apple(void) const {
    return vendor_id_ == pqrs::hid::vendor_id::value_t(0x05ac) ||
           vendor_id_ == pqrs::hid::vendor_id::value_t(0x004c);
  }

  bool operator==(const device_identifiers& other) const {
    return vendor_id_ == other.vendor_id_ &&
           product_id_ == other.product_id_ &&
           device_address_ == other.device_address_ &&
           is_keyboard_ == other.is_keyboard_ &&
           is_pointing_device_ == other.is_pointing_device_ &&
           is_game_pad_ == other.is_game_pad_;
  }

private:
  nlohmann::json json_;
  pqrs::hid::vendor_id::value_t vendor_id_;
  pqrs::hid::product_id::value_t product_id_;
  bool is_keyboard_;
  bool is_pointing_device_;
  bool is_game_pad_;
  // optional identifier
  std::string device_address_;
};

inline void to_json(nlohmann::json& json, const device_identifiers& value) {
  json = value.get_json();
  json["vendor_id"] = type_safe::get(value.get_vendor_id());
  json["product_id"] = type_safe::get(value.get_product_id());
  json["is_keyboard"] = value.get_is_keyboard();
  json["is_pointing_device"] = value.get_is_pointing_device();
  json["is_game_pad"] = value.get_is_game_pad();

  if (value.get_device_address() != "") {
    json["device_address"] = value.get_device_address();
  }
}

inline void from_json(const nlohmann::json& json, device_identifiers& value) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [k, v] : json.items()) {
    if (k == "vendor_id") {
      pqrs::json::requires_number(v, "`" + k + "`");

      value.set_vendor_id(v.get<pqrs::hid::vendor_id::value_t>());

    } else if (k == "product_id") {
      pqrs::json::requires_number(v, "`" + k + "`");

      value.set_product_id(v.get<pqrs::hid::product_id::value_t>());

    } else if (k == "device_address") {
      pqrs::json::requires_string(v, "`" + k + "`");

      value.set_device_address(v.get<std::string>());

    } else if (k == "is_keyboard") {
      pqrs::json::requires_boolean(v, "`" + k + "`");

      value.set_is_keyboard(v.get<bool>());

    } else if (k == "is_pointing_device") {
      pqrs::json::requires_boolean(v, "`" + k + "`");

      value.set_is_pointing_device(v.get<bool>());

    } else if (k == "is_game_pad") {
      pqrs::json::requires_boolean(v, "`" + k + "`");

      value.set_is_game_pad(v.get<bool>());

    } else {
      // Allow unknown key
    }
  }

  value.set_json(json);
}
} // namespace krbn
