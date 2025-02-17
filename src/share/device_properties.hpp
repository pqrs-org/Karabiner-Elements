#pragma once

#include "iokit_utility.hpp"
#include "types.hpp"
#include <gsl/gsl>
#include <pqrs/osx/iokit_hid_device.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace krbn {
class device_properties final {
public:
  struct initialization_parameters final {
    device_id device_id = krbn::device_id(0);
    pqrs::hid::vendor_id::value_t vendor_id = pqrs::hid::vendor_id::value_t(0);
    pqrs::hid::product_id::value_t product_id = pqrs::hid::product_id::value_t(0);
    location_id location_id = krbn::location_id(0);
    pqrs::hid::manufacturer_string::value_t manufacturer = pqrs::hid::manufacturer_string::value_t("");
    pqrs::hid::product_string::value_t product = pqrs::hid::product_string::value_t("");
    std::string serial_number = "";
    std::string transport = "";
    std::string device_address = "";
    bool is_keyboard = false;
    bool is_pointing_device = false;
    bool is_game_pad = false;
    bool is_consumer = false;
  };

  device_properties(initialization_parameters parameters)
      : device_id_(parameters.device_id),
        location_id_(parameters.location_id),
        manufacturer_(parameters.manufacturer),
        product_(parameters.product),
        serial_number_(parameters.serial_number),
        transport_(parameters.transport),
        is_built_in_keyboard_(false),
        is_built_in_pointing_device_(false),
        is_built_in_touch_bar_(false),
        is_apple_(false) {
    device_identifiers_ = device_identifiers(
        parameters.vendor_id,
        parameters.product_id,
        parameters.is_keyboard,
        parameters.is_pointing_device,
        parameters.is_game_pad,
        parameters.is_consumer,
        iokit_utility::is_karabiner_virtual_hid_device(manufacturer_, product_),
        parameters.device_address);

    //
    // Override manufacturer_ and product_
    //

    // Touch Bar
    if (parameters.vendor_id == pqrs::hid::vendor_id::value_t(0x05ac) &&
        parameters.product_id == pqrs::hid::product_id::value_t(0x8600)) {
      if (type_safe::get(manufacturer_).empty()) {
        manufacturer_ = pqrs::hid::manufacturer_string::value_t("Apple Inc.");
      }
      if (type_safe::get(product_).empty()) {
        product_ = pqrs::hid::product_string::value_t("Apple Internal Touch Bar");
      }
    }

    //
    // Set is_built_in_keyboard_, is_built_in_pointing_device_, is_built_in_touch_bar_
    //

    if (product_ == pqrs::hid::product_string::value_t("Apple Internal Touch Bar")) {
      is_built_in_touch_bar_ = true;
    } else if (product_ == pqrs::hid::product_string::value_t("TouchBarUserDevice")) {
      is_built_in_touch_bar_ = true;
    } else {
      if (type_safe::get(product_).find("Apple Internal ") != std::string::npos) {
        if (parameters.is_keyboard && !parameters.is_pointing_device) {
          is_built_in_keyboard_ = true;
        }
        if (!parameters.is_keyboard && parameters.is_pointing_device) {
          is_built_in_pointing_device_ = true;
        }
      }
    }

    // FIFO means the device connected via SPI (Serial Peripheral Interface)
    //
    // Note:
    // SPI devices does not have vendor_id, product_id, product_name as follows.
    // So, we have to use `transport` to determine whether the device is built-in.
    //
    // {
    //     "device_id": 4294969283,
    //     "is_keyboard": true,
    //     "is_pointing_device": false,
    //     "location_id": 161,
    //     "manufacturer": "Apple",
    //     "transport": "FIFO"
    // },
    // {
    //     "device_id": 4294969354,
    //     "is_keyboard": false,
    //     "is_pointing_device": true,
    //     "location_id": 161,
    //     "manufacturer": "Apple",
    //     "transport": "FIFO"
    // },

    if (transport_ == "FIFO") {
      if (parameters.is_keyboard) {
        is_built_in_keyboard_ = true;
      }
      if (parameters.is_pointing_device) {
        is_built_in_pointing_device_ = true;
      }
    }

    //
    // Set is_apple
    //

    if (!device_identifiers_.get_is_virtual_device()) {
      is_apple_ = parameters.vendor_id == pqrs::hid::vendor_id::value_t(0x05ac) ||
                  parameters.vendor_id == pqrs::hid::vendor_id::value_t(0x004c) ||
                  type_safe::get(product_).find("Apple Internal ") != std::string::npos ||
                  (manufacturer_ == pqrs::hid::manufacturer_string::value_t("Apple") &&
                   product_ == pqrs::hid::product_string::value_t("Headset"));
    }
  }

  device_properties(void) : device_properties(initialization_parameters{}) {
  }

  static gsl::not_null<std::shared_ptr<device_properties>> make_device_properties(device_id device_id,
                                                                                  IOHIDDeviceRef device) {
    pqrs::osx::iokit_hid_device hid_device(device);

    return std::make_shared<device_properties>(initialization_parameters{
        .device_id = device_id,
        .vendor_id = hid_device.find_vendor_id().value_or(pqrs::hid::vendor_id::value_t(0)),
        .product_id = hid_device.find_product_id().value_or(pqrs::hid::product_id::value_t(0)),
        .location_id = hid_device.find_location_id().value_or(location_id(0)),
        .manufacturer = hid_device.find_manufacturer().value_or(pqrs::hid::manufacturer_string::value_t("")),
        .product = hid_device.find_product().value_or(pqrs::hid::product_string::value_t("")),
        .serial_number = hid_device.find_serial_number().value_or(""),
        .transport = hid_device.find_transport().value_or(""),
        .device_address = hid_device.find_device_address().value_or(""),
        .is_keyboard = iokit_utility::is_keyboard(hid_device),
        .is_pointing_device = iokit_utility::is_pointing_device(hid_device),
        .is_game_pad = iokit_utility::is_game_pad(hid_device),
        .is_consumer = iokit_utility::is_consumer(hid_device),
    });
  }

  static gsl::not_null<std::shared_ptr<device_properties>> make_device_properties(const nlohmann::json& json) {
    initialization_parameters parameters;

    pqrs::json::requires_object(json, "json");

    for (const auto& [k, v] : json.items()) {
      if (k == "device_id") {
        pqrs::json::requires_number(v, "`" + k + "`");
        parameters.device_id = v.get<device_id>();

      } else if (k == "device_identifiers") {
        auto identifiers = v.get<krbn::device_identifiers>();
        parameters.vendor_id = identifiers.get_vendor_id();
        parameters.product_id = identifiers.get_product_id();
        parameters.device_address = identifiers.get_device_address();
        parameters.is_keyboard = identifiers.get_is_keyboard();
        parameters.is_pointing_device = identifiers.get_is_pointing_device();
        parameters.is_game_pad = identifiers.get_is_game_pad();
        parameters.is_consumer = identifiers.get_is_consumer();

      } else if (k == "location_id") {
        pqrs::json::requires_number(v, "`" + k + "`");
        parameters.location_id = v.get<location_id>();

      } else if (k == "manufacturer") {
        pqrs::json::requires_string(v, "`" + k + "`");
        parameters.manufacturer = v.get<pqrs::hid::manufacturer_string::value_t>();

      } else if (k == "product") {
        pqrs::json::requires_string(v, "`" + k + "`");
        parameters.product = v.get<pqrs::hid::product_string::value_t>();

      } else if (k == "serial_number") {
        pqrs::json::requires_string(v, "`" + k + "`");
        parameters.serial_number = v.get<std::string>();

      } else if (k == "transport") {
        pqrs::json::requires_string(v, "`" + k + "`");
        parameters.transport = v.get<std::string>();

      } else {
        // Allow unknown key
      }
    }

    return std::make_shared<device_properties>(parameters);
  }

  nlohmann::json to_json(void) const {
    nlohmann::json json;

    json["device_id"] = type_safe::get(device_id_);
    json["device_identifiers"] = device_identifiers_;

    if (location_id_ != location_id(0)) {
      json["location_id"] = type_safe::get(location_id_);
    }
    if (!type_safe::get(manufacturer_).empty()) {
      json["manufacturer"] = manufacturer_;
    }
    if (!type_safe::get(product_).empty()) {
      json["product"] = product_;
    }
    if (!serial_number_.empty()) {
      json["serial_number"] = serial_number_;
    }
    if (!transport_.empty()) {
      json["transport"] = transport_;
    }

    if (is_built_in_keyboard_) {
      json["is_built_in_keyboard"] = is_built_in_keyboard_;
    }
    if (is_built_in_pointing_device_) {
      json["is_built_in_pointing_device"] = is_built_in_pointing_device_;
    }
    if (is_built_in_touch_bar_) {
      json["is_built_in_touch_bar"] = is_built_in_touch_bar_;
    }
    if (is_apple_) {
      json["is_apple"] = is_apple_;
    }

    return json;
  }

  device_id get_device_id(void) const {
    return device_id_;
  }

  const device_identifiers& get_device_identifiers(void) const {
    return device_identifiers_;
  }

  location_id get_location_id(void) const {
    return location_id_;
  }

  const pqrs::hid::manufacturer_string::value_t& get_manufacturer(void) const {
    return manufacturer_;
  }

  const pqrs::hid::product_string::value_t& get_product(void) const {
    return product_;
  }

  const std::string& get_serial_number(void) const {
    return serial_number_;
  }

  const std::string& get_transport(void) const {
    return transport_;
  }

  bool get_is_built_in_keyboard(void) const {
    return is_built_in_keyboard_;
  }

  bool get_is_built_in_pointing_device(void) const {
    return is_built_in_pointing_device_;
  }

  bool get_is_built_in_touch_bar(void) const {
    return is_built_in_touch_bar_;
  }

  bool get_is_apple(void) const {
    return is_apple_;
  }

  bool compare(const device_properties& other) const {
    // product
    {
      const auto& p1 = product_;
      const auto& p2 = other.product_;
      if (p1 != p2) {
        return type_safe::get(p1).compare(type_safe::get(p2)) < 0;
      }
    }

    // manufacturer
    {
      const auto& m1 = manufacturer_;
      const auto& m2 = other.manufacturer_;
      if (m1 != m2) {
        return type_safe::get(m1).compare(type_safe::get(m2)) < 0;
      }
    }

    // is_keyboard
    {
      auto k1 = device_identifiers_.get_is_keyboard();
      auto k2 = other.device_identifiers_.get_is_keyboard();
      if (k1 != k2) {
        return k1;
      }
    }

    // is_pointing_device
    {
      auto p1 = device_identifiers_.get_is_pointing_device();
      auto p2 = other.device_identifiers_.get_is_pointing_device();
      if (p1 != p2) {
        return p1;
      }
    }

    // is_game_pad
    {
      auto p1 = device_identifiers_.get_is_game_pad();
      auto p2 = other.device_identifiers_.get_is_game_pad();
      if (p1 != p2) {
        return p1;
      }
    }

    // is_consumer
    {
      auto p1 = device_identifiers_.get_is_consumer();
      auto p2 = other.device_identifiers_.get_is_consumer();
      if (p1 != p2) {
        return p1;
      }
    }

    // device_id
    {
      auto r1 = device_id_;
      auto r2 = other.device_id_;
      if (r1 != r2) {
        return r1 < r2;
      }
    }

    return false;
  }

  bool operator==(const device_properties& other) const {
    return device_id_ == other.device_id_ &&
           device_identifiers_ == other.device_identifiers_ &&
           location_id_ == other.location_id_ &&
           manufacturer_ == other.manufacturer_ &&
           product_ == other.product_ &&
           serial_number_ == other.serial_number_ &&
           transport_ == other.transport_;
  }

private:
  device_id device_id_;
  device_identifiers device_identifiers_;
  location_id location_id_;
  pqrs::hid::manufacturer_string::value_t manufacturer_;
  pqrs::hid::product_string::value_t product_;
  std::string serial_number_;
  std::string transport_;
  bool is_built_in_keyboard_;
  bool is_built_in_pointing_device_;
  bool is_built_in_touch_bar_;
  bool is_apple_;
};

inline void to_json(nlohmann::json& json, const device_properties& device_properties) {
  json = device_properties.to_json();
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::device_properties> final {
  std::size_t operator()(const krbn::device_properties& value) const {
    std::size_t h = 0;

    // We can treat device_id_ as the unique value of device_properties.
    pqrs::hash::combine(h, value.get_device_id());

    return h;
  }
};
} // namespace std
