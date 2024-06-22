#pragma once

#include "../configuration_json_helper.hpp"
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
        selected_(false) {
    helper_values_.push_back_array<details::device>("devices",
                                                    devices_);

    //
    // Set default value
    //

    fn_function_keys_.update(make_default_fn_function_keys_json());

    //
    // Load from json
    //

    pqrs::json::requires_object(json, "json");

    helper_values_.update_value(json, error_handling);

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

    j["name"] = name_;
    j["selected"] = selected_;
    j["parameters"] = parameters_;
    j["simple_modifications"] = simple_modifications_.to_json(nlohmann::json::array());
    j["fn_function_keys"] = fn_function_keys_.to_json(make_default_fn_function_keys_json());
    j["complex_modifications"] = complex_modifications_.to_json();
    j["virtual_hid_keyboard"] = virtual_hid_keyboard_;

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
      if (d->get_identifiers() == identifiers) {
        return &(d->get_simple_modifications());
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
      if (d->get_identifiers() == identifiers) {
        return &(d->get_fn_function_keys());
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

  details::complex_modifications& get_complex_modifications(void) {
    return const_cast<details::complex_modifications&>(static_cast<const details::profile&>(*this).get_complex_modifications());
  }

  void push_front_complex_modifications_rule(const details::complex_modifications_rule& rule) {
    complex_modifications_.push_front_rule(rule);
  }

  void erase_complex_modifications_rule(size_t index) {
    complex_modifications_.erase_rule(index);
  }

  void move_complex_modifications_rule(size_t source_index, size_t destination_index) {
    complex_modifications_.move_rule(source_index, destination_index);
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

  bool get_device_disable_built_in_keyboard_if_exists(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        return d->get_disable_built_in_keyboard_if_exists();
      }
    }
    return false;
  }

  void set_device_disable_built_in_keyboard_if_exists(const device_identifiers& identifiers,
                                                      bool disable_built_in_keyboard_if_exists) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_disable_built_in_keyboard_if_exists(disable_built_in_keyboard_if_exists);
        return;
      }
    }
  }

  bool get_device_mouse_flip_x(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        return d->get_mouse_flip_x();
      }
    }
    return false;
  }

  void set_device_mouse_flip_x(const device_identifiers& identifiers,
                               bool value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_mouse_flip_x(value);
        return;
      }
    }
  }

  bool get_device_mouse_flip_y(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        return d->get_mouse_flip_y();
      }
    }
    return false;
  }

  void set_device_mouse_flip_y(const device_identifiers& identifiers,
                               bool value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_mouse_flip_y(value);
        return;
      }
    }
  }

  bool get_device_mouse_flip_vertical_wheel(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        return d->get_mouse_flip_vertical_wheel();
      }
    }
    return false;
  }

  void set_device_mouse_flip_vertical_wheel(const device_identifiers& identifiers,
                                            bool value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_mouse_flip_vertical_wheel(value);
        return;
      }
    }
  }

  bool get_device_mouse_flip_horizontal_wheel(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        return d->get_mouse_flip_horizontal_wheel();
      }
    }
    return false;
  }

  void set_device_mouse_flip_horizontal_wheel(const device_identifiers& identifiers,
                                              bool value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_mouse_flip_horizontal_wheel(value);
        return;
      }
    }
  }

  bool get_device_mouse_swap_xy(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        return d->get_mouse_swap_xy();
      }
    }
    return false;
  }

  void set_device_mouse_swap_xy(const device_identifiers& identifiers,
                                bool value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_mouse_swap_xy(value);
        return;
      }
    }
  }

  bool get_device_mouse_swap_wheels(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        return d->get_mouse_swap_wheels();
      }
    }
    return false;
  }

  void set_device_mouse_swap_wheels(const device_identifiers& identifiers,
                                    bool value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_mouse_swap_wheels(value);
        return;
      }
    }
  }

  bool get_device_mouse_discard_x(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        return d->get_mouse_discard_x();
      }
    }
    return false;
  }

  void set_device_mouse_discard_x(const device_identifiers& identifiers,
                                  bool value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_mouse_discard_x(value);
        return;
      }
    }
  }

  bool get_device_mouse_discard_y(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        return d->get_mouse_discard_y();
      }
    }
    return false;
  }

  void set_device_mouse_discard_y(const device_identifiers& identifiers,
                                  bool value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_mouse_discard_y(value);
        return;
      }
    }
  }

  bool get_device_mouse_discard_vertical_wheel(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        return d->get_mouse_discard_vertical_wheel();
      }
    }
    return false;
  }

  void set_device_mouse_discard_vertical_wheel(const device_identifiers& identifiers,
                                               bool value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_mouse_discard_vertical_wheel(value);
        return;
      }
    }
  }

  bool get_device_mouse_discard_horizontal_wheel(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        return d->get_mouse_discard_horizontal_wheel();
      }
    }
    return false;
  }

  void set_device_mouse_discard_horizontal_wheel(const device_identifiers& identifiers,
                                                 bool value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_mouse_discard_horizontal_wheel(value);
        return;
      }
    }
  }

  bool get_device_game_pad_swap_sticks(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        return d->get_game_pad_swap_sticks();
      }
    }
    return false;
  }

  void set_device_game_pad_swap_sticks(const device_identifiers& identifiers,
                                       bool value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_game_pad_swap_sticks(value);
        return;
      }
    }
  }

  //
  // game_pad_xy_stick_continued_movement_absolute_magnitude_threshold
  //

  double get_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(const device_identifiers& identifiers) const {
    auto d = get_device_or_new(identifiers);
    return d->get_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold();
  }

  void set_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(const device_identifiers& identifiers,
                                                                                    double value) {
    auto d = add_device(identifiers);
    d->set_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(value);
  }

  //
  // game_pad_xy_stick_continued_movement_interval_milliseconds
  //

  int get_device_game_pad_xy_stick_continued_movement_interval_milliseconds(const device_identifiers& identifiers) const {
    auto d = get_device_or_new(identifiers);
    return d->get_game_pad_xy_stick_continued_movement_interval_milliseconds();
  }

  void set_device_game_pad_xy_stick_continued_movement_interval_milliseconds(const device_identifiers& identifiers,
                                                                             int value) {
    auto d = add_device(identifiers);
    d->set_game_pad_xy_stick_continued_movement_interval_milliseconds(value);
  }

  //
  // game_pad_xy_stick_flicking_input_window_milliseconds
  //

  int get_device_game_pad_xy_stick_flicking_input_window_milliseconds(const device_identifiers& identifiers) const {
    auto d = get_device_or_new(identifiers);
    return d->get_game_pad_xy_stick_flicking_input_window_milliseconds();
  }

  void set_device_game_pad_xy_stick_flicking_input_window_milliseconds(const device_identifiers& identifiers,
                                                                       int value) {
    auto d = add_device(identifiers);
    d->set_game_pad_xy_stick_flicking_input_window_milliseconds(value);
  }

  //
  // game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold
  //

  double get_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(const device_identifiers& identifiers) const {
    auto d = get_device_or_new(identifiers);
    return d->get_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold();
  }

  void set_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(const device_identifiers& identifiers,
                                                                                        double value) {
    auto d = add_device(identifiers);
    d->set_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(value);
  }

  //
  // game_pad_wheels_stick_continued_movement_interval_milliseconds
  //

  int get_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(const device_identifiers& identifiers) const {
    auto d = get_device_or_new(identifiers);
    return d->get_game_pad_wheels_stick_continued_movement_interval_milliseconds();
  }

  void set_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(const device_identifiers& identifiers,
                                                                                 int value) {
    auto d = add_device(identifiers);
    d->set_game_pad_wheels_stick_continued_movement_interval_milliseconds(value);
  }

  //
  // game_pad_wheels_stick_flicking_input_window_milliseconds
  //

  int get_device_game_pad_wheels_stick_flicking_input_window_milliseconds(const device_identifiers& identifiers) const {
    auto d = get_device_or_new(identifiers);
    return d->get_game_pad_wheels_stick_flicking_input_window_milliseconds();
  }

  void set_device_game_pad_wheels_stick_flicking_input_window_milliseconds(const device_identifiers& identifiers,
                                                                           int value) {
    auto d = add_device(identifiers);
    d->set_game_pad_wheels_stick_flicking_input_window_milliseconds(value);
  }

  //
  // game_pad_stick_x_formula
  //

  bool has_device_game_pad_stick_x_formula(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        if (auto value = d->get_game_pad_stick_x_formula()) {
          return true;
        }
      }
    }
    return false;
  }

  std::string get_device_game_pad_stick_x_formula(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        if (auto value = d->get_game_pad_stick_x_formula()) {
          return *value;
        }
      }
    }
    return std::string(device::game_pad_stick_x_formula_default_value);
  }

  void set_device_game_pad_stick_x_formula(const device_identifiers& identifiers,
                                           const std::optional<std::string>& value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_game_pad_stick_x_formula(value);
        return;
      }
    }
  }

  //
  // game_pad_stick_y_formula
  //

  bool has_device_game_pad_stick_y_formula(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        if (auto value = d->get_game_pad_stick_y_formula()) {
          return true;
        }
      }
    }
    return false;
  }

  std::string get_device_game_pad_stick_y_formula(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        if (auto value = d->get_game_pad_stick_y_formula()) {
          return *value;
        }
      }
    }
    return std::string(device::game_pad_stick_y_formula_default_value);
  }

  void set_device_game_pad_stick_y_formula(const device_identifiers& identifiers,
                                           const std::optional<std::string>& value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_game_pad_stick_y_formula(value);
        return;
      }
    }
  }

  //
  // game_pad_stick_vertical_wheel_formula
  //

  bool has_device_game_pad_stick_vertical_wheel_formula(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        if (auto value = d->get_game_pad_stick_vertical_wheel_formula()) {
          return true;
        }
      }
    }
    return false;
  }

  std::string get_device_game_pad_stick_vertical_wheel_formula(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        if (auto value = d->get_game_pad_stick_vertical_wheel_formula()) {
          return *value;
        }
      }
    }
    return std::string(device::game_pad_stick_vertical_wheel_formula_default_value);
  }

  void set_device_game_pad_stick_vertical_wheel_formula(const device_identifiers& identifiers,
                                                        const std::optional<std::string>& value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_game_pad_stick_vertical_wheel_formula(value);
        return;
      }
    }
  }

  //
  // game_pad_stick_horizontal_wheel_formula
  //

  bool has_device_game_pad_stick_horizontal_wheel_formula(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        if (auto value = d->get_game_pad_stick_horizontal_wheel_formula()) {
          return true;
        }
      }
    }
    return false;
  }

  std::string get_device_game_pad_stick_horizontal_wheel_formula(const device_identifiers& identifiers) const {
    for (const auto& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        if (auto value = d->get_game_pad_stick_horizontal_wheel_formula()) {
          return *value;
        }
      }
    }
    return std::string(device::game_pad_stick_horizontal_wheel_formula_default_value);
  }

  void set_device_game_pad_stick_horizontal_wheel_formula(const device_identifiers& identifiers,
                                                          const std::optional<std::string>& value) {
    add_device(identifiers);

    for (auto&& d : devices_) {
      if (d->get_identifiers() == identifiers) {
        d->set_game_pad_stick_horizontal_wheel_formula(value);
        return;
      }
    }
  }

private:
  std::shared_ptr<details::device> find_device(const device_identifiers& identifiers) const {
    auto it = std::find_if(std::begin(devices_),
                           std::end(devices_),
                           [&](auto&& d) {
                             return d->get_identifiers() == identifiers;
                           });
    if (it != std::end(devices_)) {
      return *it;
    }
    return nullptr;
  }

  gsl::not_null<std::shared_ptr<details::device>> get_device_or_new(const device_identifiers& identifiers) const {
    if (auto d = find_device(identifiers)) {
      return d;
    }

    return add_device(identifiers);
  }

  gsl::not_null<std::shared_ptr<details::device>> add_device(const device_identifiers& identifiers) const {
    if (auto d = find_device(identifiers)) {
      return d;
    }

    devices_.push_back(std::make_shared<details::device>(nlohmann::json({
                                                             {"identifiers", identifiers},
                                                         }),
                                                         error_handling_));
    return devices_.back();
  }

  nlohmann::json json_;
  error_handling error_handling_;
  std::string name_;
  bool selected_;
  details::parameters parameters_;
  details::simple_modifications simple_modifications_;
  details::simple_modifications fn_function_keys_;
  details::complex_modifications complex_modifications_;
  details::virtual_hid_keyboard virtual_hid_keyboard_;
  mutable std::vector<gsl::not_null<std::shared_ptr<details::device>>> devices_;
  configuration_json_helper::helper_values helper_values_;
};

inline void to_json(nlohmann::json& json, const profile& profile) {
  json = profile.to_json();
}
} // namespace details
} // namespace core_configuration
} // namespace krbn
