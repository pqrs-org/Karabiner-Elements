#pragma once

#include "../configuration_json_helper.hpp"
#include "connected_devices/connected_devices.hpp"
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
  profile(const profile&) = delete;

  profile(const nlohmann::json& json,
          error_handling error_handling)
      : json_(json),
        error_handling_(error_handling),
        parameters_(std::make_shared<details::parameters>()),
        simple_modifications_(std::make_shared<simple_modifications>()),
        fn_function_keys_(std::make_shared<simple_modifications>()),
        complex_modifications_(std::make_shared<complex_modifications>()),
        virtual_hid_keyboard_(std::make_shared<details::virtual_hid_keyboard>()) {
    helper_values_.push_back_value<std::string>("name",
                                                name_,
                                                std::string(""));

    helper_values_.push_back_value<bool>("selected",
                                         selected_,
                                         false);

    helper_values_.push_back_object<details::parameters>("parameters",
                                                         parameters_);

    helper_values_.push_back_object<details::virtual_hid_keyboard>("virtual_hid_keyboard",
                                                                   virtual_hid_keyboard_);

    helper_values_.push_back_array<details::device>("devices",
                                                    devices_);

    //
    // Set default value
    //

    fn_function_keys_->update(make_default_fn_function_keys_json());

    //
    // Load from json
    //

    pqrs::json::requires_object(json, "json");

    helper_values_.update_value(json, error_handling);

    for (const auto& [key, value] : json.items()) {
      if (key == "simple_modifications") {
        try {
          simple_modifications_->update(value);
        } catch (const pqrs::json::unmarshal_error& e) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
        }

      } else if (key == "fn_function_keys") {
        try {
          fn_function_keys_->update(value);
        } catch (const pqrs::json::unmarshal_error& e) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
        }

      } else if (key == "complex_modifications") {
        try {
          complex_modifications_ = std::make_shared<complex_modifications>(value,
                                                                           error_handling);
        } catch (const pqrs::json::unmarshal_error& e) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
        }
      }
    }
  }

  static nlohmann::json make_default_fn_function_keys_json(void) {
    auto json = nlohmann::json::array();

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f1";
    json.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "display_brightness_decrement"}})});

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f2";
    json.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "display_brightness_increment"}})});

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f3";
    json.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"apple_vendor_keyboard_key_code", "mission_control"}})});

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f4";
    json.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"apple_vendor_keyboard_key_code", "spotlight"}})});

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f5";
    json.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "dictation"}})});

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f6";
    json.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"key_code", "f6"}})});

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f7";
    json.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "rewind"}})});

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f8";
    json.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "play_or_pause"}})});

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f9";
    json.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "fast_forward"}})});

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f10";
    json.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "mute"}})});

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f11";
    json.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "volume_decrement"}})});

    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f12";
    json.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "volume_increment"}})});

    return json;
  }

  nlohmann::json to_json(void) const {
    auto j = json_;

    helper_values_.update_json(j);

    {
      auto key = "simple_modifications";
      auto jj = simple_modifications_->to_json(nlohmann::json::array());
      if (!jj.empty()) {
        j[key] = jj;
      } else {
        j.erase(key);
      }
    }

    {
      auto key = "fn_function_keys";
      auto jj = fn_function_keys_->to_json(make_default_fn_function_keys_json());
      if (!jj.empty()) {
        j[key] = jj;
      } else {
        j.erase(key);
      }
    }

    {
      auto key = "complex_modifications";
      auto jj = complex_modifications_->to_json();
      if (!jj.empty()) {
        j[key] = jj;
      } else {
        j.erase(key);
      }
    }

    return j;
  }

  const std::string& get_name(void) const {
    return name_;
  }

  void set_name(const std::string& value) {
    name_ = value;
  }

  const bool& get_selected(void) const {
    return selected_;
  }

  void set_selected(bool value) {
    selected_ = value;
  }

  gsl::not_null<std::shared_ptr<details::parameters>> get_parameters(void) const {
    return parameters_;
  }

  gsl::not_null<std::shared_ptr<simple_modifications>> get_simple_modifications(void) const {
    return simple_modifications_;
  }

  gsl::not_null<std::shared_ptr<simple_modifications>> get_fn_function_keys(void) const {
    return fn_function_keys_;
  }

  gsl::not_null<std::shared_ptr<details::complex_modifications>> get_complex_modifications(void) const {
    return complex_modifications_;
  }

  gsl::not_null<std::shared_ptr<details::virtual_hid_keyboard>> get_virtual_hid_keyboard(void) const {
    return virtual_hid_keyboard_;
  }

  const std::vector<gsl::not_null<std::shared_ptr<details::device>>>& get_devices(void) const {
    return devices_;
  }

  gsl::not_null<std::shared_ptr<details::device>> get_device(const device_identifiers& identifiers) const {
    //
    // Find device
    //

    auto it = std::find_if(std::begin(devices_),
                           std::end(devices_),
                           [&](auto&& d) {
                             return d->get_identifiers() == identifiers;
                           });
    if (it != std::end(devices_)) {
      return *it;
    }

    //
    // Add device
    //

    devices_.push_back(std::make_shared<details::device>(nlohmann::json({
                                                             {"identifiers", identifiers},
                                                         }),
                                                         error_handling_));
    return devices_.back();
  }

  size_t not_connected_devices_count(const connected_devices::connected_devices& connected_devices) const {
    return std::count_if(std::begin(devices_),
                         std::end(devices_),
                         [&](auto& d) {
                           return !connected_devices.find_device(d->get_identifiers()) &&
                                  !d->to_json().empty();
                         });
  }

  void erase_not_connected_devices(const connected_devices::connected_devices& connected_devices) {
    devices_.erase(std::remove_if(std::begin(devices_),
                                  std::end(devices_),
                                  [&](auto& d) {
                                    return !connected_devices.find_device(d->get_identifiers()) &&
                                           !d->to_json().empty();
                                  }),
                   std::end(devices_));
  }

private:
  nlohmann::json json_;
  error_handling error_handling_;
  std::string name_;
  bool selected_;
  gsl::not_null<std::shared_ptr<details::parameters>> parameters_;
  gsl::not_null<std::shared_ptr<simple_modifications>> simple_modifications_;
  gsl::not_null<std::shared_ptr<simple_modifications>> fn_function_keys_;
  gsl::not_null<std::shared_ptr<details::complex_modifications>> complex_modifications_;
  gsl::not_null<std::shared_ptr<details::virtual_hid_keyboard>> virtual_hid_keyboard_;
  mutable std::vector<gsl::not_null<std::shared_ptr<details::device>>> devices_;
  configuration_json_helper::helper_values helper_values_;
};

inline void to_json(nlohmann::json& json, const profile& profile) {
  json = profile.to_json();
}
} // namespace details
} // namespace core_configuration
} // namespace krbn
