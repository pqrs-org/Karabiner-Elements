#pragma once

#include "iokit_utility.hpp"
#include "types.hpp"
#include <optional>
#include <pqrs/osx/iokit_hid_device.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace krbn {
class device_properties final {
public:
  device_properties(void) {
    update_device_identifiers();
  }

  device_properties(device_id device_id,
                    IOHIDDeviceRef device) {
    device_id_ = device_id;

    pqrs::osx::iokit_hid_device hid_device(device);

    vendor_id_ = hid_device.find_vendor_id();
    product_id_ = hid_device.find_product_id();
    location_id_ = hid_device.find_location_id();
    manufacturer_ = hid_device.find_manufacturer();
    product_ = hid_device.find_product();
    serial_number_ = hid_device.find_serial_number();
    transport_ = hid_device.find_transport();
    is_keyboard_ = iokit_utility::is_keyboard(hid_device);
    is_pointing_device_ = iokit_utility::is_pointing_device(hid_device);

    //
    // Override manufacturer_ and product_
    //

    // Touch Bar
    if (vendor_id_ == pqrs::hid::vendor_id::value_t(1452) &&
        product_id_ == pqrs::hid::product_id::value_t(34304)) {
      if (!manufacturer_) {
        manufacturer_ = "Apple Inc.";
      }
      if (!product_) {
        product_ = "Apple Internal Touch Bar";
      }
    }

    //
    // Set is_built_in_keyboard_, is_built_in_pointing_device_, is_built_in_touch_bar_
    //

    if (product_ && is_keyboard_ && is_pointing_device_) {
      if (*product_ == "Apple Internal Touch Bar") {
        is_built_in_touch_bar_ = true;
      } else if (*product_ == "TouchBarUserDevice") {
        is_built_in_touch_bar_ = true;
      } else {
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

    is_karabiner_virtual_hid_device_ = iokit_utility::is_karabiner_virtual_hid_device(device);

    update_device_identifiers();
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
    if (is_built_in_touch_bar_) {
      json["is_built_in_touch_bar"] = *is_built_in_touch_bar_;
    }
    if (is_karabiner_virtual_hid_device_) {
      json["is_karabiner_virtual_hid_device"] = *is_karabiner_virtual_hid_device_;
    }

    return json;
  }

  std::optional<pqrs::hid::vendor_id::value_t> get_vendor_id(void) const {
    return vendor_id_;
  }

  device_properties& set(pqrs::hid::vendor_id::value_t value) {
    vendor_id_ = value;
    update_device_identifiers();
    return *this;
  }

  std::optional<pqrs::hid::product_id::value_t> get_product_id(void) const {
    return product_id_;
  }

  device_properties& set(pqrs::hid::product_id::value_t value) {
    product_id_ = value;
    update_device_identifiers();
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
    update_device_identifiers();
    return *this;
  }

  std::optional<bool> get_is_pointing_device(void) const {
    return is_pointing_device_;
  }

  device_properties& set_is_pointing_device(bool value) {
    is_pointing_device_ = value;
    update_device_identifiers();
    return *this;
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

  std::optional<bool> get_is_karabiner_virtual_hid_device(void) const {
    return is_karabiner_virtual_hid_device_;
  }

  std::shared_ptr<device_identifiers> get_device_identifiers(void) const {
    return device_identifiers_;
  }

  bool compare(const device_properties& other) const {
    // product
    {
      auto p1 = product_.value_or("");
      auto p2 = other.product_.value_or("");
      if (p1 != p2) {
        return p1 < p2;
      }
    }

    // manufacturer
    {
      auto m1 = manufacturer_.value_or("");
      auto m2 = other.manufacturer_.value_or("");
      if (m1 != m2) {
        return m1 < m2;
      }
    }

    // is_keyboard
    {
      auto k1 = is_keyboard_.value_or(false);
      auto k2 = other.is_keyboard_.value_or(false);
      if (k1 != k2) {
        return k1;
      }
    }

    // is_pointing_device
    {
      auto p1 = is_pointing_device_.value_or(false);
      auto p2 = other.is_pointing_device_.value_or(false);
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
  void update_device_identifiers(void) {
    device_identifiers_ = std::make_shared<device_identifiers>(
        vendor_id_.value_or(pqrs::hid::vendor_id::value_t(0)),
        product_id_.value_or(pqrs::hid::product_id::value_t(0)),
        is_keyboard_.value_or(false),
        is_pointing_device_.value_or(false));
  }

  device_id device_id_;
  std::optional<pqrs::hid::vendor_id::value_t> vendor_id_;
  std::optional<pqrs::hid::product_id::value_t> product_id_;
  std::optional<location_id> location_id_;
  std::optional<std::string> manufacturer_;
  std::optional<std::string> product_;
  std::optional<std::string> serial_number_;
  std::optional<std::string> transport_;
  std::optional<bool> is_keyboard_;
  std::optional<bool> is_pointing_device_;
  std::optional<bool> is_built_in_keyboard_;
  std::optional<bool> is_built_in_pointing_device_;
  std::optional<bool> is_built_in_touch_bar_;
  std::optional<bool> is_karabiner_virtual_hid_device_;
  std::shared_ptr<device_identifiers> device_identifiers_;
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
    if (auto device_id = value.get_device_id()) {
      pqrs::hash::combine(h, *device_id);
    }

    return h;
  }
};
} // namespace std
