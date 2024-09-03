#pragma once

#include "iokit_utility.hpp"
#include "types.hpp"
#include <gsl/gsl>
#include <optional>
#include <pqrs/osx/iokit_hid_device.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace krbn {
class device_properties final {
public:
  struct initialization_parameters final {
    device_id device_id = krbn::device_id(0);
    std::optional<pqrs::hid::vendor_id::value_t> vendor_id;
    std::optional<pqrs::hid::product_id::value_t> product_id;
    std::optional<location_id> location_id;
    std::optional<pqrs::hid::manufacturer_string::value_t> manufacturer;
    std::optional<pqrs::hid::product_string::value_t> product;
    std::optional<std::string> serial_number;
    std::optional<std::string> transport;
    std::optional<std::string> device_address;
    bool is_keyboard = false;
    bool is_pointing_device = false;
    bool is_game_pad = false;
  };

  device_properties(initialization_parameters parameters)
      : device_id_(parameters.device_id),
        location_id_(parameters.location_id),
        manufacturer_(parameters.manufacturer),
        product_(parameters.product),
        serial_number_(parameters.serial_number),
        transport_(parameters.transport),
        device_address_(parameters.device_address) {
    bool is_virtual_device = (manufacturer_ && product_)
                                 ? iokit_utility::is_karabiner_virtual_hid_device(*manufacturer_, *product_)
                                 : false;

    device_identifiers_ = device_identifiers(
        parameters.vendor_id.value_or(pqrs::hid::vendor_id::value_t(0)),
        parameters.product_id.value_or(pqrs::hid::product_id::value_t(0)),
        parameters.is_keyboard,
        parameters.is_pointing_device,
        parameters.is_game_pad,
        is_virtual_device,
        device_address_.value_or(""));

    //
    // Override manufacturer_ and product_
    //

    // Touch Bar
    if (parameters.vendor_id == pqrs::hid::vendor_id::value_t(1452) &&
        parameters.product_id == pqrs::hid::product_id::value_t(34304)) {
      if (!manufacturer_) {
        manufacturer_ = pqrs::hid::manufacturer_string::value_t("Apple Inc.");
      }
      if (!product_) {
        product_ = pqrs::hid::product_string::value_t("Apple Internal Touch Bar");
      }
    }

    //
    // Set is_built_in_keyboard_, is_built_in_pointing_device_, is_built_in_touch_bar_
    //

    if (product_) {
      if (*product_ == pqrs::hid::product_string::value_t("Apple Internal Touch Bar")) {
        is_built_in_touch_bar_ = true;
      } else if (*product_ == pqrs::hid::product_string::value_t("TouchBarUserDevice")) {
        is_built_in_touch_bar_ = true;
      } else {
        if (type_safe::get(*product_).find("Apple Internal ") != std::string::npos) {
          if (parameters.is_keyboard && !parameters.is_pointing_device) {
            is_built_in_keyboard_ = true;
          }
          if (!parameters.is_keyboard && parameters.is_pointing_device) {
            is_built_in_pointing_device_ = true;
          }
        }
      }
    }

    if (transport_) {
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

      if (*transport_ == "FIFO") {
        if (parameters.is_keyboard) {
          is_built_in_keyboard_ = true;
        }
        if (parameters.is_pointing_device) {
          is_built_in_pointing_device_ = true;
        }
      }
    }
  }

  device_properties(void) : device_properties(initialization_parameters{}) {
  }

  static gsl::not_null<std::shared_ptr<device_properties>> make_device_properties(device_id device_id,
                                                                                  IOHIDDeviceRef device) {
    pqrs::osx::iokit_hid_device hid_device(device);

    return std::make_shared<device_properties>(initialization_parameters{
        .device_id = device_id,
        .vendor_id = hid_device.find_vendor_id(),
        .product_id = hid_device.find_product_id(),
        .location_id = hid_device.find_location_id(),
        .manufacturer = hid_device.find_manufacturer(),
        .product = hid_device.find_product(),
        .serial_number = hid_device.find_serial_number(),
        .transport = hid_device.find_transport(),
        .device_address = hid_device.find_device_address(),
        .is_keyboard = iokit_utility::is_keyboard(hid_device),
        .is_pointing_device = iokit_utility::is_pointing_device(hid_device),
        .is_game_pad = iokit_utility::is_game_pad(hid_device),
    });
  }

  nlohmann::json to_json(void) const {
    nlohmann::json json;

    json["device_id"] = type_safe::get(device_id_);
    json["device_identifiers"] = device_identifiers_;

    if (location_id_) {
      json["location_id"] = type_safe::get(*location_id_);
    }
    if (manufacturer_) {
      json["manufacturer"] = *manufacturer_;
    }
    if (product_) {
      json["product"] = *product_;
    }
    if (serial_number_) {
      json["serial_number"] = *serial_number_;
    }
    if (transport_) {
      json["transport"] = *transport_;
    }
    if (device_address_) {
      json["device_address"] = *device_address_;
    }
    if (is_built_in_keyboard_) {
      json["is_built_in_keyboard"] = *is_built_in_keyboard_;
    }
    if (is_built_in_pointing_device_) {
      json["is_built_in_pointing_device"] = *is_built_in_pointing_device_;
    }
    if (is_built_in_touch_bar_) {
      json["is_built_in_touch_bar"] = *is_built_in_touch_bar_;
    }

    return json;
  }

  device_id get_device_id(void) const {
    return device_id_;
  }

  const device_identifiers& get_device_identifiers(void) const {
    return device_identifiers_;
  }

  std::optional<location_id> get_location_id(void) const {
    return location_id_;
  }

  std::optional<pqrs::hid::manufacturer_string::value_t> get_manufacturer(void) const {
    return manufacturer_;
  }

  std::optional<pqrs::hid::product_string::value_t> get_product(void) const {
    return product_;
  }

  std::optional<std::string> get_serial_number(void) const {
    return serial_number_;
  }

  std::optional<std::string> get_transport(void) const {
    return transport_;
  }

  std::optional<std::string> get_device_address(void) const {
    return device_address_;
  }

  std::optional<bool> get_is_built_in_keyboard(void) const {
    return is_built_in_keyboard_;
  }

  std::optional<bool> get_is_built_in_pointing_device(void) const {
    return is_built_in_pointing_device_;
  }

  std::optional<bool> get_is_built_in_touch_bar(void) const {
    return is_built_in_touch_bar_;
  }

  bool compare(const device_properties& other) const {
    // product
    {
      auto p1 = product_.value_or(pqrs::hid::product_string::value_t(""));
      auto p2 = other.product_.value_or(pqrs::hid::product_string::value_t(""));
      if (p1 != p2) {
        return type_safe::get(p1).compare(type_safe::get(p2)) < 0;
      }
    }

    // manufacturer
    {
      auto m1 = manufacturer_.value_or(pqrs::hid::manufacturer_string::value_t(""));
      auto m2 = other.manufacturer_.value_or(pqrs::hid::manufacturer_string::value_t(""));
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
           transport_ == other.transport_ &&
           device_address_ == other.device_address_;
  }

private:
  device_id device_id_;
  device_identifiers device_identifiers_;
  std::optional<location_id> location_id_;
  std::optional<pqrs::hid::manufacturer_string::value_t> manufacturer_;
  std::optional<pqrs::hid::product_string::value_t> product_;
  std::optional<std::string> serial_number_;
  std::optional<std::string> transport_;
  std::optional<std::string> device_address_;
  std::optional<bool> is_built_in_keyboard_;
  std::optional<bool> is_built_in_pointing_device_;
  std::optional<bool> is_built_in_touch_bar_;
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
