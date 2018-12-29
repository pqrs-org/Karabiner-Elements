#pragma once

#include "device_properties_manager.hpp"
#include "manipulator/details/conditions/base.hpp"
#include <optional>
#include <string>
#include <vector>

namespace krbn {
namespace manipulator {
namespace details {
namespace conditions {
class device final : public base {
public:
  enum class type {
    device_if,
    device_unless,
  };

  device(const nlohmann::json& json) : base(),
                                       type_(type::device_if) {
    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "type") {
          if (value.is_string()) {
            if (value == "device_if") {
              type_ = type::device_if;
            }
            if (value == "device_unless") {
              type_ = type::device_unless;
            }
          }
        } else if (key == "identifiers") {
          if (value.is_array()) {
            handle_identifiers_json(value);
          } else {
            logger::get_logger()->error("complex_modifications json error: {0} should be array {1}", key, json.dump());
          }
        } else if (key == "description") {
          // Do nothing
        } else {
          logger::get_logger()->error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }
    }
  }

  virtual ~device(void) {
  }

  virtual bool is_fulfilled(const event_queue::entry& entry,
                            const manipulator_environment& manipulator_environment) const {
    if (!definitions_.empty()) {
      if (auto dp = manipulator_environment.find_device_properties(entry.get_device_id())) {
        for (const auto& d : definitions_) {
          bool fulfilled = true;

          if (d.vendor_id && d.vendor_id != dp->get_vendor_id()) {
            fulfilled = false;
          }
          if (d.product_id && d.product_id != dp->get_product_id()) {
            fulfilled = false;
          }
          if (d.location_id && d.location_id != dp->get_location_id()) {
            fulfilled = false;
          }
          if (d.is_keyboard && d.is_keyboard != dp->get_is_keyboard()) {
            fulfilled = false;
          }
          if (d.is_pointing_device && d.is_pointing_device != dp->get_is_pointing_device()) {
            fulfilled = false;
          }

          if (fulfilled) {
            switch (type_) {
              case type::device_if:
                return true;
              case type::device_unless:
                return false;
            }
          }
        }
      }
    }

    // Not found

    switch (type_) {
      case type::device_if:
        return false;
      case type::device_unless:
        return true;
    }
  }

private:
  struct definition final {
    std::optional<vendor_id> vendor_id;
    std::optional<product_id> product_id;
    std::optional<location_id> location_id;
    std::optional<bool> is_keyboard;
    std::optional<bool> is_pointing_device;
  };

  void handle_identifiers_json(const nlohmann::json& json) {
    for (const auto& j : json) {
      if (j.is_object()) {
        definition d;

        for (auto it = std::begin(j); it != std::end(j); std::advance(it, 1)) {
          // it.key() is always std::string.
          const auto& key = it.key();
          const auto& value = it.value();

          if (key == "vendor_id") {
            if (value.is_number()) {
              d.vendor_id = vendor_id(value.get<int>());
            } else {
              logger::get_logger()->error("complex_modifications json error: Invalid form of {0} in {1}", key, j.dump());
            }
          } else if (key == "product_id") {
            if (value.is_number()) {
              d.product_id = product_id(value.get<int>());
            } else {
              logger::get_logger()->error("complex_modifications json error: Invalid form of {0} in {1}", key, j.dump());
            }
          } else if (key == "location_id") {
            if (value.is_number()) {
              d.location_id = location_id(value.get<int>());
            } else {
              logger::get_logger()->error("complex_modifications json error: Invalid form of {0} in {1}", key, j.dump());
            }
          } else if (key == "is_keyboard") {
            if (value.is_boolean()) {
              d.is_keyboard = value.get<bool>();
            } else {
              logger::get_logger()->error("complex_modifications json error: Invalid form of {0} in {1}", key, j.dump());
            }
          } else if (key == "is_pointing_device") {
            if (value.is_boolean()) {
              d.is_pointing_device = value.get<bool>();
            } else {
              logger::get_logger()->error("complex_modifications json error: Invalid form of {0} in {1}", key, j.dump());
            }
          } else if (key == "description") {
            // Do nothing
          } else {
            logger::get_logger()->error("complex_modifications json error: Unknown key: {0} in {1}", key, j.dump());
          }
        }

        if (d.vendor_id) {
          definitions_.push_back(d);
        }

      } else {
        logger::get_logger()->error("complex_modifications json error: `identifiers` children should be object {0}", json.dump());
      }
    }
  }

  type type_;
  std::vector<definition> definitions_;
};
} // namespace conditions
} // namespace details
} // namespace manipulator
} // namespace krbn
