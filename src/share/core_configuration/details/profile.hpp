#pragma once

#include "profile/complex_modifications.hpp"
#include "profile/device.hpp"
#include "profile/parameters.hpp"
#include "profile/simple_modifications.hpp"
#include "profile/virtual_hid_keyboard.hpp"
#include <pqrs/json.hpp>

namespace krbn {
namespace core_configuration {
namespace details {
class profile final {
public:
  profile(const nlohmann::json& json) : json_(json),
                                        selected_(false) {
    // ----------------------------------------
    // Set default value

    fn_function_keys_.update(make_default_fn_function_keys_json());

    // ----------------------------------------
    // Load from json

    pqrs::json::requires_object(json, "json");

    for (const auto& [key, value] : json.items()) {
      if (key == "name") {
        pqrs::json::requires_string(value, "`" + key + "`");

        name_ = value.get<std::string>();

      } else if (key == "selected") {
        pqrs::json::requires_boolean(value, "`" + key + "`");

        selected_ = value.get<bool>();

      } else if (key == "parameters") {
        try {
          parameters_ = value.get<parameters>();
        } catch (const pqrs::json::unmarshal_error& e) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
        }

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

      } else if (key == "complex_modifications") {
        try {
          complex_modifications_ = complex_modifications(value);
        } catch (const pqrs::json::unmarshal_error& e) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
        }

      } else if (key == "virtual_hid_keyboard") {
        try {
          virtual_hid_keyboard_ = value.get<virtual_hid_keyboard>();
        } catch (const pqrs::json::unmarshal_error& e) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
        }

      } else if (key == "devices") {
        pqrs::json::requires_array(value, "`" + key + "`");

        for (const auto& j : value) {
          try {
            devices_.emplace_back(j);
          } catch (const pqrs::json::unmarshal_error& e) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` entry error: {1}", key, e.what()));
          }
        }
      }
    }
  }

  static nlohmann::json make_default_fn_function_keys_json(void) {
    auto json = nlohmann::json::array();

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f1";
    json.back()["to"]["consumer_key_code"] = "display_brightness_decrement";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f2";
    json.back()["to"]["consumer_key_code"] = "display_brightness_increment";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f3";
    json.back()["to"]["key_code"] = "mission_control";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f4";
    json.back()["to"]["key_code"] = "launchpad";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f5";
    json.back()["to"]["key_code"] = "illumination_decrement";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f6";
    json.back()["to"]["key_code"] = "illumination_increment";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f7";
    json.back()["to"]["consumer_key_code"] = "rewind";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f8";
    json.back()["to"]["consumer_key_code"] = "play_or_pause";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f9";
    json.back()["to"]["consumer_key_code"] = "fast_forward";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f10";
    json.back()["to"]["consumer_key_code"] = "mute";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f11";
    json.back()["to"]["consumer_key_code"] = "volume_decrement";

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f12";
    json.back()["to"]["consumer_key_code"] = "volume_increment";

    return json;
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["name"] = name_;
    j["selected"] = selected_;
    j["parameters"] = parameters_;
    j["simple_modifications"] = simple_modifications_.to_json();
    j["fn_function_keys"] = fn_function_keys_.to_json();
    j["complex_modifications"] = complex_modifications_.to_json();
    j["virtual_hid_keyboard"] = virtual_hid_keyboard_;
    j["devices"] = devices_;
    return j;
  }

  const std::string& get_name(void) const {
    return name_;
  }

  void set_name(const std::string& value) {
    name_ = value;
  }

  bool get_selected(void) const {
    return selected_;
  }

  void set_selected(bool value) {
    selected_ = value;
  }

  const details::parameters& get_parameters(void) const {
    return parameters_;
  }

  details::parameters& get_parameters(void) {
    return const_cast<details::parameters&>(static_cast<const profile&>(*this).get_parameters());
  }

  const details::simple_modifications& get_simple_modifications(void) const {
    return simple_modifications_;
  }

  details::simple_modifications& get_simple_modifications(void) {
    return const_cast<details::simple_modifications&>(static_cast<const profile&>(*this).get_simple_modifications());
  }

  const details::simple_modifications* find_simple_modifications(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d.get_identifiers() == identifiers) {
        return &(d.get_simple_modifications());
      }
    }
    return nullptr;
  }

  details::simple_modifications* find_simple_modifications(const device_identifiers& identifiers) {
    add_device(identifiers);

    return const_cast<details::simple_modifications*>(static_cast<const profile&>(*this).find_simple_modifications(identifiers));
  }

  const details::simple_modifications& get_fn_function_keys(void) const {
    return fn_function_keys_;
  }

  details::simple_modifications& get_fn_function_keys(void) {
    return const_cast<details::simple_modifications&>(static_cast<const profile&>(*this).get_fn_function_keys());
  }

  const details::simple_modifications* find_fn_function_keys(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d.get_identifiers() == identifiers) {
        return &(d.get_fn_function_keys());
      }
    }
    return nullptr;
  }

  details::simple_modifications* find_fn_function_keys(const device_identifiers& identifiers) {
    add_device(identifiers);

    return const_cast<details::simple_modifications*>(static_cast<const profile&>(*this).find_fn_function_keys(identifiers));
  }

  const details::complex_modifications& get_complex_modifications(void) const {
    return complex_modifications_;
  }

  void push_back_complex_modifications_rule(const details::complex_modifications_rule& rule) {
    complex_modifications_.push_back_rule(rule);
  }

  void erase_complex_modifications_rule(size_t index) {
    complex_modifications_.erase_rule(index);
  }

  void swap_complex_modifications_rules(size_t index1, size_t index2) {
    complex_modifications_.swap_rules(index1, index2);
  }

  void set_complex_modifications_parameter(const std::string& name, int value) {
    complex_modifications_.set_parameter_value(name, value);
  }

  const details::virtual_hid_keyboard& get_virtual_hid_keyboard(void) const {
    return virtual_hid_keyboard_;
  }

  details::virtual_hid_keyboard& get_virtual_hid_keyboard(void) {
    return virtual_hid_keyboard_;
  }

  const std::vector<details::device>& get_devices(void) const {
    return devices_;
  }

  bool get_device_ignore(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d.get_identifiers() == identifiers) {
        return d.get_ignore();
      }
    }

    details::device d(nlohmann::json({
        {"identifiers", identifiers},
    }));
    return d.get_ignore();
  }

  void set_device_ignore(const device_identifiers& identifiers,
                         bool ignore) {
    add_device(identifiers);

    for (auto&& device : devices_) {
      if (device.get_identifiers() == identifiers) {
        device.set_ignore(ignore);
        return;
      }
    }
  }

  bool get_device_manipulate_caps_lock_led(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d.get_identifiers() == identifiers) {
        return d.get_manipulate_caps_lock_led();
      }
    }

    details::device d(nlohmann::json({
        {"identifiers", identifiers},
    }));
    return d.get_manipulate_caps_lock_led();
  }

  void set_device_manipulate_caps_lock_led(const device_identifiers& identifiers,
                                           bool manipulate_caps_lock_led) {
    add_device(identifiers);

    for (auto&& device : devices_) {
      if (device.get_identifiers() == identifiers) {
        device.set_manipulate_caps_lock_led(manipulate_caps_lock_led);
        return;
      }
    }
  }

  bool get_device_disable_built_in_keyboard_if_exists(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d.get_identifiers() == identifiers) {
        return d.get_disable_built_in_keyboard_if_exists();
      }
    }
    return false;
  }

  void set_device_disable_built_in_keyboard_if_exists(const device_identifiers& identifiers,
                                                      bool disable_built_in_keyboard_if_exists) {
    add_device(identifiers);

    for (auto&& device : devices_) {
      if (device.get_identifiers() == identifiers) {
        device.set_disable_built_in_keyboard_if_exists(disable_built_in_keyboard_if_exists);
        return;
      }
    }
  }

private:
  void add_device(const device_identifiers& identifiers) {
    for (auto&& device : devices_) {
      if (device.get_identifiers() == identifiers) {
        return;
      }
    }

    auto json = nlohmann::json({
        {"identifiers", identifiers},
    });
    devices_.emplace_back(json);
  }

  nlohmann::json json_;
  std::string name_;
  bool selected_;
  details::parameters parameters_;
  details::simple_modifications simple_modifications_;
  details::simple_modifications fn_function_keys_;
  details::complex_modifications complex_modifications_;
  details::virtual_hid_keyboard virtual_hid_keyboard_;
  std::vector<details::device> devices_;
};

inline void to_json(nlohmann::json& json, const profile& profile) {
  json = profile.to_json();
}
} // namespace details
} // namespace core_configuration
} // namespace krbn
