#pragma once

#include "base.hpp"
#include "device_properties_manager.hpp"
#include <optional>
#include <string>
#include <vector>

namespace krbn {
namespace manipulator {
namespace conditions {
class device final : public base {
public:
  enum class type {
    device_if,
    device_unless,
  };

  device(const nlohmann::json& json) : base(),
                                       type_(type::device_if) {
    if (!json.is_object()) {
      throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
    }

    for (const auto& [key, value] : json.items()) {
      // key is always std::string.

      if (key == "type") {
        if (!value.is_string()) {
          throw pqrs::json::unmarshal_error(fmt::format("{0} must be string, but is `{1}`", key, value.dump()));
        }

        auto t = value.get<std::string>();

        if (t == "device_if") {
          type_ = type::device_if;
        } else if (t == "device_unless") {
          type_ = type::device_unless;
        } else {
          throw pqrs::json::unmarshal_error(fmt::format("unknown type `{0}`", t));
        }

      } else if (key == "identifiers") {
        if (!value.is_array()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be array, but is `{1}`", key, value.dump()));
        }

        handle_identifiers_json(value);

      } else if (key == "description") {
        // Do nothing

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, json.dump()));
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
      if (!j.is_object()) {
        throw pqrs::json::unmarshal_error(
            fmt::format("identifiers entry must be object, but is `{0}`", j.dump()));
      }

      definition d;

      for (const auto& [key, value] : j.items()) {
        // key is always std::string.

        if (key == "vendor_id") {
          if (!value.is_number()) {
            throw pqrs::json::unmarshal_error(
                fmt::format("identifiers entry `{0}` must be number, but is `{1}`", key, value.dump()));
          }

          d.vendor_id = vendor_id(value.get<int>());

        } else if (key == "product_id") {
          if (!value.is_number()) {
            throw pqrs::json::unmarshal_error(
                fmt::format("identifiers entry `{0}` must be number, but is `{1}`", key, value.dump()));
          }

          d.product_id = product_id(value.get<int>());

        } else if (key == "location_id") {
          if (!value.is_number()) {
            throw pqrs::json::unmarshal_error(
                fmt::format("identifiers entry `{0}` must be number, but is `{1}`", key, value.dump()));
          }

          d.location_id = location_id(value.get<int>());

        } else if (key == "is_keyboard") {
          if (!value.is_boolean()) {
            throw pqrs::json::unmarshal_error(
                fmt::format("identifiers entry `{0}` must be boolean, but is `{1}`", key, value.dump()));
          }

          d.is_keyboard = value.get<bool>();

        } else if (key == "is_pointing_device") {
          if (!value.is_boolean()) {
            throw pqrs::json::unmarshal_error(
                fmt::format("identifiers entry `{0}` must be boolean, but is `{1}`", key, value.dump()));
          }

          d.is_pointing_device = value.get<bool>();

        } else if (key == "description") {
          // Do nothing

        } else {
          throw pqrs::json::unmarshal_error(
              fmt::format("unknown key `{0}` in `{1}`", key, j.dump()));
        }
      }

      if (!d.vendor_id) {
        throw pqrs::json::unmarshal_error(
            fmt::format("`vendor_id` must be specified: `{0}`", j.dump()));
      }

      definitions_.push_back(d);
    }
  }

  type type_;
  std::vector<definition> definitions_;
};
} // namespace conditions
} // namespace manipulator
} // namespace krbn
