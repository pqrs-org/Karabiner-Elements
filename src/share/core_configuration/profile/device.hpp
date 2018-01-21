#pragma once

class device final {
public:
  device(const nlohmann::json& json) : json_(json),
                                       identifiers_(json_utility::find_copy(json, "identifiers", nlohmann::json())),
                                       ignore_(false),
                                       manipulate_caps_lock_led_(false),
                                       disable_built_in_keyboard_if_exists_(false),
                                       simple_modifications_(json_utility::find_copy(json, "simple_modifications", nlohmann::json::array())),
                                       fn_function_keys_(make_default_fn_function_keys_json()) {
    // ----------------------------------------
    // Set default value

    // ignore_

    if (identifiers_.get_is_pointing_device()) {
      ignore_ = true;
    } else if (identifiers_.get_vendor_id() == vendor_id(0x05ac) &&
               identifiers_.get_product_id() == product_id(0x8600)) {
      // Touch Bar on MacBook Pro 2016
      ignore_ = true;
    } else if (identifiers_.get_vendor_id() == vendor_id(0x1050)) {
      // YubiKey token
      ignore_ = true;
    }

    // manipulate_caps_lock_led_

    if (identifiers_.get_is_keyboard() &&
        identifiers_.is_apple()) {
      manipulate_caps_lock_led_ = true;
    }

    // ----------------------------------------
    // Load from json

    if (auto v = json_utility::find_optional<bool>(json, "ignore")) {
      ignore_ = *v;
    }

    if (auto v = json_utility::find_optional<bool>(json, "manipulate_caps_lock_led")) {
      manipulate_caps_lock_led_ = *v;
    }

    if (auto v = json_utility::find_optional<bool>(json, "disable_built_in_keyboard_if_exists")) {
      disable_built_in_keyboard_if_exists_ = *v;
    }

    if (auto v = json_utility::find_json(json, "fn_function_keys")) {
      fn_function_keys_.update(*v);
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
    j["simple_modifications"] = simple_modifications_;
    j["fn_function_keys"] = fn_function_keys_;
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
