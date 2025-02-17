#pragma once

#include "base.hpp"
#include "device_properties_manager.hpp"
#include "device_utility.hpp"
#include <optional>
#include <pqrs/hid.hpp>
#include <pqrs/hid/extra/nlohmann_json.hpp>
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
    device_exists_if,
    device_exists_unless,
  };

  device(const nlohmann::json& json) : base(),
                                       type_(type::device_if) {
    pqrs::json::requires_object(json, "json");

    for (const auto& [key, value] : json.items()) {
      // key is always std::string.

      if (key == "type") {
        pqrs::json::requires_string(value, key);

        auto t = value.get<std::string>();

        if (t == "device_if") {
          type_ = type::device_if;
        } else if (t == "device_unless") {
          type_ = type::device_unless;
        } else if (t == "device_exists_if") {
          type_ = type::device_exists_if;
        } else if (t == "device_exists_unless") {
          type_ = type::device_exists_unless;
        } else {
          throw pqrs::json::unmarshal_error(fmt::format("unknown type `{0}`", t));
        }

      } else if (key == "identifiers") {
        pqrs::json::requires_array(value, "`identifiers`");

        handle_identifiers_json(value);

      } else if (key == "description") {
        // Do nothing

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, pqrs::json::dump_for_error_message(json)));
      }
    }
  }

  virtual ~device(void) {
  }

  virtual bool is_fulfilled(const event_queue::entry& entry,
                            const manipulator_environment& manipulator_environment) const {
    if (!definitions_.empty()) {
      switch (type_) {
        case type::device_if:
        case type::device_unless:
          if (auto dp = manipulator_environment.find_device_properties(entry.get_device_id())) {
            for (const auto& d : definitions_) {
              if (d.fulfilled(*dp, manipulator_environment)) {
                switch (type_) {
                  case type::device_if:
                  case type::device_exists_if:
                    return true;
                  case type::device_unless:
                  case type::device_exists_unless:
                    return false;
                }
              }
            }
          }
          break;

        case type::device_exists_if:
        case type::device_exists_unless:
          for (const auto& [device_id, dp] : manipulator_environment.get_device_properties_manager().get_map()) {
            if (dp) {
              for (const auto& d : definitions_) {
                if (d.fulfilled(*dp, manipulator_environment)) {
                  switch (type_) {
                    case type::device_if:
                    case type::device_exists_if:
                      return true;
                    case type::device_unless:
                    case type::device_exists_unless:
                      return false;
                  }
                }
              }
            }
          }
          break;
      }
    }

    // Not found

    switch (type_) {
      case type::device_if:
      case type::device_exists_if:
        return false;
      case type::device_unless:
      case type::device_exists_unless:
        return true;
    }
  }

private:
  struct definition final {
    std::optional<pqrs::hid::vendor_id::value_t> vendor_id;
    std::optional<pqrs::hid::product_id::value_t> product_id;
    std::optional<location_id> location_id;
    std::optional<std::string> device_address;
    std::optional<bool> is_keyboard;
    std::optional<bool> is_pointing_device;
    std::optional<bool> is_game_pad;
    std::optional<bool> is_consumer;
    std::optional<bool> is_touch_bar;
    std::optional<bool> is_built_in_keyboard;

    bool valid(void) const {
      return vendor_id ||
             product_id ||
             location_id ||
             device_address ||
             is_keyboard ||
             is_pointing_device ||
             is_game_pad ||
             is_consumer ||
             is_touch_bar ||
             is_built_in_keyboard;
    }

    bool fulfilled(const device_properties& device_properties,
                   const manipulator_environment& manipulator_environment) const {
      //
      // If `device_properties.vendor_id` is std::nullopt, it is treated as 0 in `devices.identifiers` in `karabiner.json`, so std::nullopt is also treated as 0 here.
      // Other properties are treated in the same way.
      //

      if (vendor_id && vendor_id != device_properties.get_device_identifiers().get_vendor_id()) {
        return false;
      }
      if (product_id && product_id != device_properties.get_device_identifiers().get_product_id()) {
        return false;
      }
      if (location_id && location_id != device_properties.get_location_id()) {
        return false;
      }
      if (device_address && device_address != device_properties.get_device_identifiers().get_device_address()) {
        return false;
      }
      if (is_keyboard && is_keyboard != device_properties.get_device_identifiers().get_is_keyboard()) {
        return false;
      }
      if (is_pointing_device && is_pointing_device != device_properties.get_device_identifiers().get_is_pointing_device()) {
        return false;
      }
      if (is_game_pad && is_game_pad != device_properties.get_device_identifiers().get_is_game_pad()) {
        return false;
      }
      if (is_consumer && is_consumer != device_properties.get_device_identifiers().get_is_consumer()) {
        return false;
      }
      if (is_touch_bar && is_touch_bar != device_properties.get_is_built_in_touch_bar()) {
        return false;
      }
      if (is_built_in_keyboard) {
        auto c = manipulator_environment.get_core_configuration();
        if (is_built_in_keyboard != device_utility::determine_is_built_in_keyboard(*c, device_properties)) {
          return false;
        }
      }

      return true;
    }
  };

  void handle_identifiers_json(const nlohmann::json& json) {
    for (const auto& j : json) {
      pqrs::json::requires_object(j, "identifiers entry");

      definition d;

      for (const auto& [key, value] : j.items()) {
        // key is always std::string.

        if (key == "vendor_id") {
          pqrs::json::requires_number(value, "identifiers entry `vendor_id`");

          d.vendor_id = value.get<pqrs::hid::vendor_id::value_t>();

        } else if (key == "product_id") {
          pqrs::json::requires_number(value, "identifiers entry `product_id`");

          d.product_id = value.get<pqrs::hid::product_id::value_t>();

        } else if (key == "location_id") {
          pqrs::json::requires_number(value, "identifiers entry `location_id`");

          d.location_id = location_id(value.get<int>());

        } else if (key == "device_address") {
          pqrs::json::requires_string(value, "identifiers entry `device_address`");

          d.device_address = value.get<std::string>();

        } else if (key == "is_keyboard") {
          pqrs::json::requires_boolean(value, "identifiers entry `is_keyboard`");

          d.is_keyboard = value.get<bool>();

        } else if (key == "is_pointing_device") {
          pqrs::json::requires_boolean(value, "identifiers entry `is_pointing_device`");

          d.is_pointing_device = value.get<bool>();

        } else if (key == "is_game_pad") {
          pqrs::json::requires_boolean(value, "identifiers entry `is_game_pad`");

          d.is_game_pad = value.get<bool>();

        } else if (key == "is_consumer") {
          pqrs::json::requires_boolean(value, "identifiers entry `is_consumer`");

          d.is_consumer = value.get<bool>();

        } else if (key == "is_touch_bar") {
          pqrs::json::requires_boolean(value, "identifiers entry `is_touch_bar`");

          d.is_touch_bar = value.get<bool>();

        } else if (key == "is_built_in_keyboard") {
          pqrs::json::requires_boolean(value, "identifiers entry `is_built_in_keyboard`");

          d.is_built_in_keyboard = value.get<bool>();

        } else if (key == "description") {
          // Do nothing

        } else {
          throw pqrs::json::unmarshal_error(
              fmt::format("unknown key `{0}` in `{1}`", key, pqrs::json::dump_for_error_message(j)));
        }
      }

      if (d.valid()) {
        definitions_.push_back(d);
      }
    }
  }

  type type_;
  std::vector<definition> definitions_;
};
} // namespace conditions
} // namespace manipulator
} // namespace krbn
