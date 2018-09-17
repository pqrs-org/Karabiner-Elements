#pragma once

#include "types/product_id.hpp"
#include "types/vendor_id.hpp"
#include <cstdint>
#include <json/json.hpp>

namespace krbn {
class device_identifiers final {
public:
  device_identifiers(void) : vendor_id_(vendor_id(0)),
                             product_id_(product_id(0)),
                             is_keyboard_(false),
                             is_pointing_device_(false) {
  }

  device_identifiers(vendor_id vendor_id,
                     product_id product_id,
                     bool is_keyboard,
                     bool is_pointing_device) : vendor_id_(vendor_id),
                                                product_id_(product_id),
                                                is_keyboard_(is_keyboard),
                                                is_pointing_device_(is_pointing_device) {
  }

  device_identifiers(const nlohmann::json& json) : device_identifiers() {
    json_ = json;

    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "vendor_id") {
          if (value.is_number()) {
            vendor_id_ = vendor_id(static_cast<uint32_t>(value));
          } else {
            logger::get_logger().error("Invalid form of {0}: {1}", key, value.dump());
          }
        }
        if (key == "product_id") {
          if (value.is_number()) {
            product_id_ = product_id(static_cast<uint32_t>(value));
          } else {
            logger::get_logger().error("Invalid form of {0}: {1}", key, value.dump());
          }
        }
        if (key == "is_keyboard") {
          if (value.is_boolean()) {
            is_keyboard_ = value;
          } else {
            logger::get_logger().error("Invalid form of {0}: {1}", key, value.dump());
          }
        }
        if (key == "is_pointing_device") {
          if (value.is_boolean()) {
            is_pointing_device_ = value;
          } else {
            logger::get_logger().error("Invalid form of {0}: {1}", key, value.dump());
          }
        }
      }

    } else {
      logger::get_logger().error("Invalid form of device_identifiers: {0}", json.dump());
    }
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["vendor_id"] = static_cast<uint32_t>(vendor_id_);
    j["product_id"] = static_cast<uint32_t>(product_id_);
    j["is_keyboard"] = is_keyboard_;
    j["is_pointing_device"] = is_pointing_device_;
    return j;
  }

  vendor_id get_vendor_id(void) const {
    return vendor_id_;
  }

  product_id get_product_id(void) const {
    return product_id_;
  }

  bool get_is_keyboard(void) const {
    return is_keyboard_;
  }

  bool get_is_pointing_device(void) const {
    return is_pointing_device_;
  }

  bool is_apple(void) const {
    return vendor_id_ == vendor_id(0x05ac) ||
           vendor_id_ == vendor_id(0x004c);
  }

  bool operator==(const device_identifiers& other) const {
    return vendor_id_ == other.vendor_id_ &&
           product_id_ == other.product_id_ &&
           is_keyboard_ == other.is_keyboard_ &&
           is_pointing_device_ == other.is_pointing_device_;
  }

private:
  nlohmann::json json_;
  vendor_id vendor_id_;
  product_id product_id_;
  bool is_keyboard_;
  bool is_pointing_device_;
};

inline void to_json(nlohmann::json& json, const device_identifiers& identifiers) {
  json = identifiers.to_json();
}
} // namespace krbn
