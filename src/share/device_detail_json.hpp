#pragma once

#include "boost_defs.hpp"

#include "types.hpp"
#include <boost/optional.hpp>

namespace krbn {
class device_detail_json final {
public:
  device_detail_json(boost::optional<vendor_id> vendor_id,
                     boost::optional<product_id> product_id,
                     boost::optional<location_id> location_id,
                     boost::optional<std::string> manufacturer,
                     boost::optional<std::string> product,
                     boost::optional<std::string> serial_number,
                     boost::optional<std::string> transport,
                     boost::optional<uint64_t> registry_entry_id) {
    if (vendor_id) {
      json_["vendor_id"] = static_cast<int>(*vendor_id);
    }
    if (product_id) {
      json_["product_id"] = static_cast<int>(*product_id);
    }
    if (location_id) {
      json_["location_id"] = static_cast<int>(*location_id);
    }
    if (manufacturer) {
      json_["manufacturer"] = *manufacturer;
    }
    if (product) {
      json_["product"] = *product;
    }
    if (serial_number) {
      json_["serial_number"] = *serial_number;
    }
    if (transport) {
      json_["transport"] = *transport;
    }
    if (registry_entry_id) {
      json_["registry_entry_id"] = *registry_entry_id;
    }
  }

  const nlohmann::json& get_json(void) {
    return json_;
  }

private:
  nlohmann::json json_;
};
} // namespace krbn
