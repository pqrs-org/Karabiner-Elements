#pragma once

#include "boost_defs.hpp"

#include "json_utility.hpp"
#include "types.hpp"
#include <boost/optional.hpp>

namespace krbn {
class device_detail final {
public:
  device_detail(boost::optional<vendor_id> vendor_id,
                boost::optional<product_id> product_id,
                boost::optional<location_id> location_id,
                boost::optional<std::string> manufacturer,
                boost::optional<std::string> product,
                boost::optional<std::string> serial_number,
                boost::optional<std::string> transport,
                boost::optional<uint64_t> registry_entry_id,
                bool is_keyboard,
                bool is_pointing_device) : vendor_id_(vendor_id),
                                           product_id_(product_id),
                                           location_id_(location_id),
                                           manufacturer_(manufacturer),
                                           product_(product),
                                           serial_number_(serial_number),
                                           transport_(transport),
                                           registry_entry_id_(registry_entry_id),
                                           is_keyboard_(is_keyboard),
                                           is_pointing_device_(is_pointing_device) {
  }

  nlohmann::json to_json(void) const {
    nlohmann::json json;

    if (vendor_id_) {
      json["vendor_id"] = static_cast<int>(*vendor_id_);
    }
    if (product_id_) {
      json["product_id"] = static_cast<int>(*product_id_);
    }
    if (location_id_) {
      json["location_id"] = static_cast<int>(*location_id_);
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
    if (registry_entry_id_) {
      json["registry_entry_id"] = *registry_entry_id_;
    }
    json["is_keyboard"] = is_keyboard_;
    json["is_pointing_device"] = is_pointing_device_;

    return json;
  }

  bool compare(const device_detail& other) {
    // product
    {
      auto p1 = get_product_string();
      auto p2 = other.get_product_string();
      if (p1 != p2) {
        return p1 < p2;
      }
    }

    // manufacturer
    {
      auto m1 = get_manufacturer_string();
      auto m2 = other.get_manufacturer_string();
      if (m1 != m2) {
        return m1 < m2;
      }
    }

    // is_keyboard
    {
      if (is_keyboard_ != other.is_keyboard_) {
        return is_keyboard_;
      }
    }

    // is_pointing_device
    {
      if (is_pointing_device_ != other.is_pointing_device_) {
        return is_pointing_device_;
      }
    }

    // registry_entry_id
    {
      auto r1 = get_registry_entry_id_number();
      auto r2 = other.get_registry_entry_id_number();
      if (r1 != r2) {
        return r1 < r2;
      }
    }

    return false;
  }

private:
  std::string get_manufacturer_string(void) const {
    if (manufacturer_) {
      return *manufacturer_;
    }
    return "";
  }

  std::string get_product_string(void) const {
    if (product_) {
      return *product_;
    }
    return "";
  }

  uint64_t get_registry_entry_id_number(void) const {
    if (registry_entry_id_) {
      return *registry_entry_id_;
    }
    return 0;
  }

  boost::optional<vendor_id> vendor_id_;
  boost::optional<product_id> product_id_;
  boost::optional<location_id> location_id_;
  boost::optional<std::string> manufacturer_;
  boost::optional<std::string> product_;
  boost::optional<std::string> serial_number_;
  boost::optional<std::string> transport_;
  boost::optional<uint64_t> registry_entry_id_;
  bool is_keyboard_;
  bool is_pointing_device_;
};

inline void to_json(nlohmann::json& json, const device_detail& device_detail) {
  json = device_detail.to_json();
}
} // namespace krbn
