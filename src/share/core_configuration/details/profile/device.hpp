#pragma once

#include "simple_modifications.hpp"

namespace krbn {
namespace core_configuration {
namespace details {
class device final {
public:
  device(const nlohmann::json& json) : json_(json),
                                       ignore_(false),
                                       manipulate_caps_lock_led_(false),
                                       disable_built_in_keyboard_if_exists_(false) {
    auto ignore_configured = false;
    auto manipulate_caps_lock_led_configured = false;

    // ----------------------------------------
    // Set default value

    // fn_function_keys_

    fn_function_keys_.update(make_default_fn_function_keys_json());

    // ----------------------------------------
    // Load from json

    pqrs::json::requires_object(json, "json");

    for (const auto& [key, value] : json.items()) {
      if (key == "identifiers") {
        try {
          identifiers_ = value.get<device_identifiers>();
        } catch (const pqrs::json::unmarshal_error& e) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
        }

      } else if (key == "ignore") {
        pqrs::json::requires_boolean(value, "`" + key + "`");

        ignore_ = value.get<bool>();
        ignore_configured = true;

      } else if (key == "manipulate_caps_lock_led") {
        pqrs::json::requires_boolean(value, "`" + key + "`");

        manipulate_caps_lock_led_ = value.get<bool>();
        manipulate_caps_lock_led_configured = true;

      } else if (key == "disable_built_in_keyboard_if_exists") {
        pqrs::json::requires_boolean(value, "`" + key + "`");

        disable_built_in_keyboard_if_exists_ = value.get<bool>();

      } else if (key == "simple_modifications") {
        try {
          simple_modifications_.update(value);
        } catch (const pqrs::json::unmarshal_error& e) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
        }

      } else if (key == "fn_function_keys") {
        try {
          fn_function_keys_.update(value);
        } catch (const pqrs::json::unmarshal_error& e) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
        }
      }
    }

    // ----------------------------------------
    // Set special default value for specific devices.

    // ignore_

    if (!ignore_configured) {
      if (identifiers_.get_is_pointing_device()) {
        ignore_ = true;
      } else if (identifiers_.get_vendor_id() == pqrs::hid::vendor_id::value_t(0x05ac) &&
                 identifiers_.get_product_id() == pqrs::hid::product_id::value_t(0x8600)) {
        // Touch Bar on MacBook Pro 2016
        ignore_ = true;
      } else if (identifiers_.get_vendor_id() == pqrs::hid::vendor_id::value_t(0x1050)) {
        // YubiKey token
        ignore_ = true;
      }
    }

    // manipulate_caps_lock_led_

    if (!manipulate_caps_lock_led_configured) {
      if (identifiers_.get_is_keyboard() &&
          identifiers_.is_apple()) {
        manipulate_caps_lock_led_ = true;
      }
    }
  }

  static nlohmann::json make_default_fn_function_keys_json(void) {
    auto json = nlohmann::json::array();

    for (int i = 1; i <= 12; ++i) {
      json.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"key_code", fmt::format("f{0}", i)}})},
          {"to", nlohmann::json::object()},
      }));
    }

    return json;
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["identifiers"] = identifiers_;
    j["ignore"] = ignore_;
    j["manipulate_caps_lock_led"] = manipulate_caps_lock_led_;
    j["disable_built_in_keyboard_if_exists"] = disable_built_in_keyboard_if_exists_;
    j["simple_modifications"] = simple_modifications_.to_json();
    j["fn_function_keys"] = fn_function_keys_.to_json();
    return j;
  }

  const device_identifiers& get_identifiers(void) const {
    return identifiers_;
  }

  bool get_ignore(void) const {
    return ignore_;
  }
  void set_ignore(bool value) {
    ignore_ = value;
  }

  bool get_manipulate_caps_lock_led(void) const {
    return manipulate_caps_lock_led_;
  }
  void set_manipulate_caps_lock_led(bool value) {
    manipulate_caps_lock_led_ = value;
  }

  bool get_disable_built_in_keyboard_if_exists(void) const {
    return disable_built_in_keyboard_if_exists_;
  }
  void set_disable_built_in_keyboard_if_exists(bool value) {
    disable_built_in_keyboard_if_exists_ = value;
  }

  const simple_modifications& get_simple_modifications(void) const {
    return simple_modifications_;
  }
  simple_modifications& get_simple_modifications(void) {
    return simple_modifications_;
  }

  const simple_modifications& get_fn_function_keys(void) const {
    return fn_function_keys_;
  }
  simple_modifications& get_fn_function_keys(void) {
    return fn_function_keys_;
  }

private:
  nlohmann::json json_;
  device_identifiers identifiers_;
  bool ignore_;
  bool manipulate_caps_lock_led_;
  bool disable_built_in_keyboard_if_exists_;
  simple_modifications simple_modifications_;
  simple_modifications fn_function_keys_;
};

inline void to_json(nlohmann::json& json, const device& device) {
  json = device.to_json();
}
} // namespace details
} // namespace core_configuration
} // namespace krbn
