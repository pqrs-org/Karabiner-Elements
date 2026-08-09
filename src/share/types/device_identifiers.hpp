#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <pqrs/hid.hpp>
#include <pqrs/hid/extra/nlohmann_json.hpp>

namespace krbn {
class device_identifiers final {
public:
  struct initialization_parameters final {
    pqrs::hid::vendor_id::value_t vendor_id = pqrs::hid::vendor_id::value_t(0);
    pqrs::hid::product_id::value_t product_id = pqrs::hid::product_id::value_t(0);
    bool is_keyboard = false;
    bool is_pointing_device = false;
    bool is_game_pad = false;
    bool is_consumer = false;
    bool is_virtual_device = false;
    std::string device_address;
  };

  device_identifiers()
      : device_identifiers(initialization_parameters{}) {
  }

  explicit device_identifiers(const initialization_parameters& parameters)
      : json_(nlohmann::json::object()),
        vendor_id_(parameters.vendor_id),
        product_id_(parameters.product_id),
        is_keyboard_(parameters.is_keyboard),
        is_pointing_device_(parameters.is_pointing_device),
        is_game_pad_(parameters.is_game_pad),
        is_consumer_(parameters.is_consumer),
        is_virtual_device_(parameters.is_virtual_device) {
    // Some bluetooth devices do not have a vendor_id or product_id.
    // Such devices use device_address to distinguish between devices.
    // The device_address will be changed when the hardware is replaced,
    // so we use device_address only when vendor_id and product_id are zero.
    if (parameters.vendor_id == pqrs::hid::vendor_id::value_t(0) &&
        parameters.product_id == pqrs::hid::product_id::value_t(0)) {
      device_address_ = parameters.device_address;
    }
  }

  device_identifiers(const nlohmann::json& json)
      : device_identifiers() {
    json_ = json;

    pqrs::json::requires_object(json, "json");

    for (const auto& [k, v] : json.items()) {
      if (k == "vendor_id") {
        pqrs::json::requires_number(v, "`" + k + "`");

        vendor_id_ = v.get<pqrs::hid::vendor_id::value_t>();

      } else if (k == "product_id") {
        pqrs::json::requires_number(v, "`" + k + "`");

        product_id_ = v.get<pqrs::hid::product_id::value_t>();

      } else if (k == "device_address") {
        pqrs::json::requires_string(v, "`" + k + "`");

        device_address_ = v.get<std::string>();

      } else if (k == "is_keyboard") {
        pqrs::json::requires_boolean(v, "`" + k + "`");

        is_keyboard_ = v.get<bool>();

      } else if (k == "is_pointing_device") {
        pqrs::json::requires_boolean(v, "`" + k + "`");

        is_pointing_device_ = v.get<bool>();

      } else if (k == "is_game_pad") {
        pqrs::json::requires_boolean(v, "`" + k + "`");

        is_game_pad_ = v.get<bool>();

      } else if (k == "is_consumer") {
        pqrs::json::requires_boolean(v, "`" + k + "`");

        is_consumer_ = v.get<bool>();

      } else if (k == "is_virtual_device") {
        pqrs::json::requires_boolean(v, "`" + k + "`");

        is_virtual_device_ = v.get<bool>();

      } else {
        // Allow unknown key
      }
    }
  }

  [[nodiscard]] const nlohmann::json& get_json() const {
    return json_;
  }

  [[nodiscard]] pqrs::hid::vendor_id::value_t get_vendor_id() const {
    return vendor_id_;
  }

  [[nodiscard]] pqrs::hid::product_id::value_t get_product_id() const {
    return product_id_;
  }

  [[nodiscard]] bool get_is_keyboard() const {
    return is_keyboard_;
  }

  [[nodiscard]] bool get_is_pointing_device() const {
    return is_pointing_device_;
  }

  [[nodiscard]] bool get_is_game_pad() const {
    return is_game_pad_;
  }

  [[nodiscard]] bool get_is_consumer() const {
    return is_consumer_;
  }

  [[nodiscard]] bool get_is_virtual_device() const {
    return is_virtual_device_;
  }

  [[nodiscard]] const std::string& get_device_address() const {
    return device_address_;
  }

  // Returns a canonical representation based only on the values that define
  // device identity. Unlike to_json, this excludes unknown source JSON keys
  // and normalizes device_address according to the constructor rules.
  [[nodiscard]] nlohmann::json to_normalized_json() const;

  [[nodiscard]] bool empty() const {
    return vendor_id_ == pqrs::hid::vendor_id::value_t(0) &&
           product_id_ == pqrs::hid::product_id::value_t(0) &&
           !is_keyboard_ &&
           !is_pointing_device_ &&
           !is_game_pad_ &&
           !is_consumer_ &&
           !is_virtual_device_ &&
           device_address_.empty();
  }

  bool operator==(const device_identifiers& other) const {
    return vendor_id_ == other.vendor_id_ &&
           product_id_ == other.product_id_ &&
           is_keyboard_ == other.is_keyboard_ &&
           is_pointing_device_ == other.is_pointing_device_ &&
           is_game_pad_ == other.is_game_pad_ &&
           is_consumer_ == other.is_consumer_ &&
           is_virtual_device_ == other.is_virtual_device_ &&
           device_address_ == other.device_address_;
  }

  //
  // Helper methods
  //

  [[nodiscard]] bool is_nintendo_pro_controller_0x057e_0x2009() const {
    return get_vendor_id() == pqrs::hid::vendor_id::value_t(0x057e) &&
           get_product_id() == pqrs::hid::product_id::value_t(0x2009);
  }

private:
  nlohmann::json json_;
  pqrs::hid::vendor_id::value_t vendor_id_;
  pqrs::hid::product_id::value_t product_id_;
  bool is_keyboard_;
  bool is_pointing_device_;
  bool is_game_pad_;
  bool is_consumer_;
  bool is_virtual_device_;
  // optional identifier
  std::string device_address_;
};

inline void to_json(nlohmann::json& json, const device_identifiers& value) {
  json = value.get_json();

  {
    auto key = "vendor_id";
    auto v = type_safe::get(value.get_vendor_id());
    if (v != 0) {
      json[key] = v;
    } else {
      json.erase(key);
    }
  }

  {
    auto key = "product_id";
    auto v = type_safe::get(value.get_product_id());
    if (v != 0) {
      json[key] = v;
    } else {
      json.erase(key);
    }
  }

  {
    auto key = "is_keyboard";
    auto v = value.get_is_keyboard();
    if (v != false) {
      json[key] = v;
    } else {
      json.erase(key);
    }
  }

  {
    auto key = "is_pointing_device";
    auto v = value.get_is_pointing_device();
    if (v != false) {
      json[key] = v;
    } else {
      json.erase(key);
    }
  }

  {
    auto key = "is_game_pad";
    auto v = value.get_is_game_pad();
    if (v != false) {
      json[key] = v;
    } else {
      json.erase(key);
    }
  }

  {
    auto key = "is_consumer";
    auto v = value.get_is_consumer();
    if (v != false) {
      json[key] = v;
    } else {
      json.erase(key);
    }
  }

  {
    auto key = "is_virtual_device";
    auto v = value.get_is_virtual_device();
    if (v != false) {
      json[key] = v;
    } else {
      json.erase(key);
    }
  }

  {
    auto key = "device_address";
    auto v = value.get_device_address();
    if (v != "") {
      json[key] = v;
    } else {
      json.erase(key);
    }
  }
}

inline nlohmann::json device_identifiers::to_normalized_json() const {
  return device_identifiers({
      .vendor_id = vendor_id_,
      .product_id = product_id_,
      .is_keyboard = is_keyboard_,
      .is_pointing_device = is_pointing_device_,
      .is_game_pad = is_game_pad_,
      .is_consumer = is_consumer_,
      .is_virtual_device = is_virtual_device_,
      .device_address = device_address_,
  });
}

inline void from_json(const nlohmann::json& json, device_identifiers& value) {
  value = device_identifiers(json);
}
} // namespace krbn
