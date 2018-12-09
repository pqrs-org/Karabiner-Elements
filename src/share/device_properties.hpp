#pragma once

#include "iokit_utility.hpp"
#include "json_utility.hpp"
#include "types.hpp"
#include <optional>
#include <pqrs/osx/iokit_hid_device.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace krbn {
class device_properties final {
public:
  device_properties(void) {
  }

  device_properties(device_id device_id,
                    IOHIDDeviceRef device) {
    device_id_ = device_id;

    pqrs::osx::iokit_hid_device hid_device(device);

    if (auto value = hid_device.find_vendor_id()) {
      vendor_id_ = make_vendor_id(*value);
    }
    if (auto value = hid_device.find_product_id()) {
      product_id_ = make_product_id(*value);
    }
    if (auto value = hid_device.find_location_id()) {
      location_id_ = make_location_id(*value);
    }
    manufacturer_ = hid_device.find_manufacturer();
    product_ = hid_device.find_product();
    serial_number_ = hid_device.find_serial_number();
    transport_ = hid_device.find_transport();
    is_keyboard_ = iokit_utility::is_keyboard(device);
    is_pointing_device_ = iokit_utility::is_pointing_device(device);

    if (product_ && is_keyboard_ && is_pointing_device_) {
      if ((*product_).find("Apple Internal ") != std::string::npos) {
        if (*is_keyboard_ == true && *is_pointing_device_ == false) {
          is_built_in_keyboard_ = true;
        }
        if (*is_keyboard_ == false && *is_pointing_device_ == true) {
          is_built_in_pointing_device_ = true;
        }
      }
    }
  }

  nlohmann::json to_json(void) const {
    nlohmann::json json;

    json["device_id"] = type_safe::get(device_id_);

    if (vendor_id_) {
      json["vendor_id"] = type_safe::get(*vendor_id_);
    }
    if (product_id_) {
      json["product_id"] = type_safe::get(*product_id_);
    }
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
    if (is_keyboard_) {
      json["is_keyboard"] = *is_keyboard_;
    }
    if (is_pointing_device_) {
      json["is_pointing_device"] = *is_pointing_device_;
    }
    if (is_built_in_keyboard_) {
      json["is_built_in_keyboard"] = *is_built_in_keyboard_;
    }
    if (is_built_in_pointing_device_) {
      json["is_built_in_pointing_device"] = *is_built_in_pointing_device_;
    }

    return json;
  }

  std::optional<vendor_id> get_vendor_id(void) const {
    return vendor_id_;
  }

  device_properties& set(vendor_id value) {
    vendor_id_ = value;
    return *this;
  }

  std::optional<product_id> get_product_id(void) const {
    return product_id_;
  }

  device_properties& set(product_id value) {
    product_id_ = value;
    return *this;
  }

  std::optional<location_id> get_location_id(void) const {
    return location_id_;
  }

  device_properties& set(location_id value) {
    location_id_ = value;
    return *this;
  }

  std::optional<std::string> get_manufacturer(void) const {
    return manufacturer_;
  }

  device_properties& set_manufacturer(const std::string& value) {
    manufacturer_ = value;
    return *this;
  }

  std::optional<std::string> get_product(void) const {
    return product_;
  }

  device_properties& set_product(const std::string& value) {
    product_ = value;
    return *this;
  }

  std::optional<std::string> get_serial_number(void) const {
    return serial_number_;
  }

  device_properties& set_serial_number(const std::string& value) {
    serial_number_ = value;
    return *this;
  }

  std::optional<std::string> get_transport(void) const {
    return transport_;
  }

  device_properties& set_transport(const std::string& value) {
    transport_ = value;
    return *this;
  }

  std::optional<device_id> get_device_id(void) const {
    return device_id_;
  }

  device_properties& set(device_id value) {
    device_id_ = value;
    return *this;
  }

  std::optional<bool> get_is_keyboard(void) const {
    return is_keyboard_;
  }

  device_properties& set_is_keyboard(bool value) {
    is_keyboard_ = value;
    return *this;
  }

  std::optional<bool> get_is_pointing_device(void) const {
    return is_pointing_device_;
  }

  device_properties& set_is_pointing_device(bool value) {
    is_pointing_device_ = value;
    return *this;
  }

  bool compare(const device_properties& other) {
    // product
    {
      auto p1 = make_product_value();
      auto p2 = other.make_product_value();
      if (p1 != p2) {
        return p1 < p2;
      }
    }

    // manufacturer
    {
      auto m1 = make_manufacturer_value();
      auto m2 = other.make_manufacturer_value();
      if (m1 != m2) {
        return m1 < m2;
      }
    }

    // is_keyboard
    {
      auto k1 = make_is_keyboard_value();
      auto k2 = other.make_is_keyboard_value();
      if (k1 != k2) {
        return k1;
      }
    }

    // is_pointing_device
    {
      auto p1 = make_is_pointing_device_value();
      auto p2 = other.make_is_pointing_device_value();
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
           vendor_id_ == other.vendor_id_ &&
           product_id_ == other.product_id_ &&
           location_id_ == other.location_id_ &&
           manufacturer_ == other.manufacturer_ &&
           product_ == other.product_ &&
           serial_number_ == other.serial_number_ &&
           transport_ == other.transport_ &&
           is_keyboard_ == other.is_keyboard_ &&
           is_pointing_device_ == other.is_pointing_device_;
  }

private:
  std::string make_manufacturer_value(void) const {
    if (manufacturer_) {
      return *manufacturer_;
    }
    return "";
  }

  std::string make_product_value(void) const {
    if (product_) {
      return *product_;
    }
    return "";
  }

  bool make_is_keyboard_value(void) const {
    if (is_keyboard_) {
      return *is_keyboard_;
    }
    return false;
  }

  bool make_is_pointing_device_value(void) const {
    if (is_pointing_device_) {
      return *is_pointing_device_;
    }
    return false;
  }

  device_id device_id_;
  std::optional<vendor_id> vendor_id_;
  std::optional<product_id> product_id_;
  std::optional<location_id> location_id_;
  std::optional<std::string> manufacturer_;
  std::optional<std::string> product_;
  std::optional<std::string> serial_number_;
  std::optional<std::string> transport_;
  std::optional<bool> is_keyboard_;
  std::optional<bool> is_pointing_device_;
  std::optional<bool> is_built_in_keyboard_;
  std::optional<bool> is_built_in_pointing_device_;
};

inline void to_json(nlohmann::json& json, const device_properties& device_properties) {
  json = device_properties.to_json();
}

inline size_t hash_value(const device_properties& value) {
  // We can treat device_id_ as the unique value of device_properties.
  if (auto id = value.get_device_id()) {
    return std::hash<device_id>{}(*id);
  }
  return 0;
}
} // namespace krbn
