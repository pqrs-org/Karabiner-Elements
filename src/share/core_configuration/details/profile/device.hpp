#pragma once

#include "../../configuration_json_helper.hpp"
#include "exprtk_utility.hpp"
#include "simple_modifications.hpp"
#include <pqrs/string.hpp>
#include <ranges>
#include <string_view>

namespace krbn {
namespace core_configuration {
namespace details {
class device final {
public:
  // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
  static constexpr const char* game_pad_stick_x_formula_default_value =
      "cos(radian) * delta_magnitude * 32;";

  // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
  static constexpr const char* game_pad_stick_y_formula_default_value =
      "sin(radian) * delta_magnitude * 32;";

  // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
  static constexpr const char* game_pad_stick_vertical_wheel_formula_default_value = R"(

if (abs(cos(radian)) >= abs(sin(radian))) {
  0;
} else {
  var m := 0;
  if (absolute_magnitude < 1.0) {
    m := max(0.05, delta_magnitude * 5);
  } else {
    if (delta_magnitude > 0.3) {
      m := 0.5;
    } else {
      m := 0.3;
    }
  }
  sin(radian) * m;
}

)";

  // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
  static constexpr const char* game_pad_stick_horizontal_wheel_formula_default_value = R"(

if (abs(cos(radian)) <= abs(sin(radian))) {
  0;
} else {
  var m := 0;
  if (absolute_magnitude < 1.0) {
    m := max(0.05, delta_magnitude * 5);
  } else {
    if (delta_magnitude > 0.3) {
      m := 0.5;
    } else {
      m := 0.3;
    }
  }
  cos(radian) * m;
}

)";

  device(const device&) = delete;

  device(void)
      : device(nlohmann::json::object(),
               krbn::core_configuration::error_handling::loose) {
  }

  device(const nlohmann::json& json,
         error_handling error_handling)
      : json_(json) {
    helper_values_.push_back_value<bool>("ignore",
                                         ignore_,
                                         false);

    helper_values_.push_back_value<bool>("manipulate_caps_lock_led",
                                         manipulate_caps_lock_led_,
                                         false);

    helper_values_.push_back_value<bool>("treat_as_built_in_keyboard",
                                         treat_as_built_in_keyboard_,
                                         false);

    helper_values_.push_back_value<bool>("disable_built_in_keyboard_if_exists",
                                         disable_built_in_keyboard_if_exists_,
                                         false);

    helper_values_.push_back_value<bool>("mouse_flip_x",
                                         mouse_flip_x_,
                                         false);

    helper_values_.push_back_value<bool>("mouse_flip_y",
                                         mouse_flip_y_,
                                         false);

    helper_values_.push_back_value<bool>("mouse_flip_vertical_wheel",
                                         mouse_flip_vertical_wheel_,
                                         false);

    helper_values_.push_back_value<bool>("mouse_flip_horizontal_wheel",
                                         mouse_flip_horizontal_wheel_,
                                         false);

    helper_values_.push_back_value<bool>("mouse_swap_xy",
                                         mouse_swap_xy_,
                                         false);

    helper_values_.push_back_value<bool>("mouse_swap_wheels",
                                         mouse_swap_wheels_,
                                         false);

    helper_values_.push_back_value<bool>("mouse_discard_x",
                                         mouse_discard_x_,
                                         false);

    helper_values_.push_back_value<bool>("mouse_discard_y",
                                         mouse_discard_y_,
                                         false);

    helper_values_.push_back_value<bool>("mouse_discard_vertical_wheel",
                                         mouse_discard_vertical_wheel_,
                                         false);

    helper_values_.push_back_value<bool>("mouse_discard_horizontal_wheel",
                                         mouse_discard_horizontal_wheel_,
                                         false);

    helper_values_.push_back_value<bool>("game_pad_swap_sticks",
                                         game_pad_swap_sticks_,
                                         false);

    helper_values_.push_back_value<double>("game_pad_xy_stick_continued_movement_absolute_magnitude_threshold",
                                           game_pad_xy_stick_continued_movement_absolute_magnitude_threshold_,
                                           1.0);

    helper_values_.push_back_value<int>("game_pad_xy_stick_continued_movement_interval_milliseconds",
                                        game_pad_xy_stick_continued_movement_interval_milliseconds_,
                                        20);

    helper_values_.push_back_value<int>("game_pad_xy_stick_flicking_input_window_milliseconds",
                                        game_pad_xy_stick_flicking_input_window_milliseconds_,
                                        50);

    helper_values_.push_back_value<double>("game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold",
                                           game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold_,
                                           1.0);

    helper_values_.push_back_value<int>("game_pad_wheels_stick_continued_movement_interval_milliseconds",
                                        game_pad_wheels_stick_continued_movement_interval_milliseconds_,
                                        10);

    helper_values_.push_back_value<int>("game_pad_wheels_stick_flicking_input_window_milliseconds",
                                        game_pad_wheels_stick_flicking_input_window_milliseconds_,
                                        50);

    // ----------------------------------------
    // Set default value

    // fn_function_keys_

    fn_function_keys_.update(make_default_fn_function_keys_json());

    // ----------------------------------------
    // Load from json

    pqrs::json::requires_object(json, "json");

    helper_values_.update_value(json, error_handling);

    for (const auto& [key, value] : json.items()) {
      if (key == "identifiers") {
        try {
          identifiers_ = value.get<device_identifiers>();
        } catch (const pqrs::json::unmarshal_error& e) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
        }

      } else if (key == "game_pad_stick_x_formula") {
        game_pad_stick_x_formula_ = unmarshal_formula(value, key);

      } else if (key == "game_pad_stick_y_formula") {
        game_pad_stick_y_formula_ = unmarshal_formula(value, key);

      } else if (key == "game_pad_stick_vertical_wheel_formula") {
        game_pad_stick_vertical_wheel_formula_ = unmarshal_formula(value, key);

      } else if (key == "game_pad_stick_horizontal_wheel_formula") {
        game_pad_stick_horizontal_wheel_formula_ = unmarshal_formula(value, key);

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

    //
    // Set special default value for specific devices.
    //

    // ignore_

    if (identifiers_.get_is_pointing_device() ||
        identifiers_.get_is_game_pad() ||
        // YubiKey token
        identifiers_.get_vendor_id() == pqrs::hid::vendor_id::value_t(0x1050)) {
      helper_values_.set_default_value(ignore_,
                                       true);
    }

    // manipulate_caps_lock_led_

    if (identifiers_.get_is_keyboard()) {
      helper_values_.set_default_value(manipulate_caps_lock_led_,
                                       true);
    }

    //
    // Coordinate between settings.
    //

    coordinate_between_properties();
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

    helper_values_.update_json(j);

    j["identifiers"] = identifiers_;

#define OPTIONAL_SETTING(name)     \
  {                                \
    if (name##_ != std::nullopt) { \
      j[#name] = *name##_;         \
    } else {                       \
      j.erase(#name);              \
    }                              \
  }

#undef OPTIONAL_SETTING

#define OPTIONAL_FORMULA(name)              \
  {                                         \
    if (name##_ != std::nullopt) {          \
      j[#name] = marshal_formula(*name##_); \
    } else {                                \
      j.erase(#name);                       \
    }                                       \
  }

    OPTIONAL_FORMULA(game_pad_stick_x_formula);
    OPTIONAL_FORMULA(game_pad_stick_y_formula);
    OPTIONAL_FORMULA(game_pad_stick_vertical_wheel_formula);
    OPTIONAL_FORMULA(game_pad_stick_horizontal_wheel_formula);

#undef OPTIONAL_FORMULA

    {
      auto jj = simple_modifications_.to_json(nlohmann::json::array());
      if (!jj.empty()) {
        j["simple_modifications"] = jj;
      } else {
        j.erase("simple_modifications");
      }
    }

    {
      auto jj = fn_function_keys_.to_json(make_default_fn_function_keys_json());
      if (!jj.empty()) {
        j["fn_function_keys"] = jj;
      } else {
        j.erase("fn_function_keys");
      }
    }

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

    coordinate_between_properties();
  }

  bool get_manipulate_caps_lock_led(void) const {
    return manipulate_caps_lock_led_;
  }
  void set_manipulate_caps_lock_led(bool value) {
    manipulate_caps_lock_led_ = value;

    coordinate_between_properties();
  }

  bool get_treat_as_built_in_keyboard(void) const {
    return treat_as_built_in_keyboard_;
  }
  void set_treat_as_built_in_keyboard(bool value) {
    treat_as_built_in_keyboard_ = value;

    coordinate_between_properties();
  }

  bool get_disable_built_in_keyboard_if_exists(void) const {
    return disable_built_in_keyboard_if_exists_;
  }
  void set_disable_built_in_keyboard_if_exists(bool value) {
    disable_built_in_keyboard_if_exists_ = value;

    coordinate_between_properties();
  }

  bool get_mouse_flip_x(void) const {
    return mouse_flip_x_;
  }
  void set_mouse_flip_x(bool value) {
    mouse_flip_x_ = value;

    coordinate_between_properties();
  }

  bool get_mouse_flip_y(void) const {
    return mouse_flip_y_;
  }
  void set_mouse_flip_y(bool value) {
    mouse_flip_y_ = value;

    coordinate_between_properties();
  }

  bool get_mouse_flip_vertical_wheel(void) const {
    return mouse_flip_vertical_wheel_;
  }
  void set_mouse_flip_vertical_wheel(bool value) {
    mouse_flip_vertical_wheel_ = value;

    coordinate_between_properties();
  }

  bool get_mouse_flip_horizontal_wheel(void) const {
    return mouse_flip_horizontal_wheel_;
  }
  void set_mouse_flip_horizontal_wheel(bool value) {
    mouse_flip_horizontal_wheel_ = value;

    coordinate_between_properties();
  }

  bool get_mouse_swap_xy(void) const {
    return mouse_swap_xy_;
  }
  void set_mouse_swap_xy(bool value) {
    mouse_swap_xy_ = value;

    coordinate_between_properties();
  }

  bool get_mouse_swap_wheels(void) const {
    return mouse_swap_wheels_;
  }
  void set_mouse_swap_wheels(bool value) {
    mouse_swap_wheels_ = value;

    coordinate_between_properties();
  }

  bool get_mouse_discard_x(void) const {
    return mouse_discard_x_;
  }
  void set_mouse_discard_x(bool value) {
    mouse_discard_x_ = value;

    coordinate_between_properties();
  }

  bool get_mouse_discard_y(void) const {
    return mouse_discard_y_;
  }
  void set_mouse_discard_y(bool value) {
    mouse_discard_y_ = value;

    coordinate_between_properties();
  }

  bool get_mouse_discard_vertical_wheel(void) const {
    return mouse_discard_vertical_wheel_;
  }
  void set_mouse_discard_vertical_wheel(bool value) {
    mouse_discard_vertical_wheel_ = value;

    coordinate_between_properties();
  }

  bool get_mouse_discard_horizontal_wheel(void) const {
    return mouse_discard_horizontal_wheel_;
  }
  void set_mouse_discard_horizontal_wheel(bool value) {
    mouse_discard_horizontal_wheel_ = value;

    coordinate_between_properties();
  }

  bool get_game_pad_swap_sticks(void) const {
    return game_pad_swap_sticks_;
  }
  void set_game_pad_swap_sticks(bool value) {
    game_pad_swap_sticks_ = value;

    coordinate_between_properties();
  }

  const double& get_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(void) const {
    return game_pad_xy_stick_continued_movement_absolute_magnitude_threshold_;
  }
  void set_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(double value) {
    game_pad_xy_stick_continued_movement_absolute_magnitude_threshold_ = value;

    coordinate_between_properties();
  }

  const int& get_game_pad_xy_stick_continued_movement_interval_milliseconds(void) const {
    return game_pad_xy_stick_continued_movement_interval_milliseconds_;
  }
  void set_game_pad_xy_stick_continued_movement_interval_milliseconds(int value) {
    game_pad_xy_stick_continued_movement_interval_milliseconds_ = value;

    coordinate_between_properties();
  }

  const int& get_game_pad_xy_stick_flicking_input_window_milliseconds(void) const {
    return game_pad_xy_stick_flicking_input_window_milliseconds_;
  }
  void set_game_pad_xy_stick_flicking_input_window_milliseconds(int value) {
    game_pad_xy_stick_flicking_input_window_milliseconds_ = value;

    coordinate_between_properties();
  }

  const double& get_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(void) const {
    return game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold_;
  }
  void set_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(double value) {
    game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold_ = value;

    coordinate_between_properties();
  }

  const int& get_game_pad_wheels_stick_continued_movement_interval_milliseconds(void) const {
    return game_pad_wheels_stick_continued_movement_interval_milliseconds_;
  }
  void set_game_pad_wheels_stick_continued_movement_interval_milliseconds(int value) {
    game_pad_wheels_stick_continued_movement_interval_milliseconds_ = value;

    coordinate_between_properties();
  }

  const int& get_game_pad_wheels_stick_flicking_input_window_milliseconds(void) const {
    return game_pad_wheels_stick_flicking_input_window_milliseconds_;
  }
  void set_game_pad_wheels_stick_flicking_input_window_milliseconds(int value) {
    game_pad_wheels_stick_flicking_input_window_milliseconds_ = value;

    coordinate_between_properties();
  }

  const std::optional<std::string>& get_game_pad_stick_x_formula(void) const {
    return game_pad_stick_x_formula_;
  }
  void set_game_pad_stick_x_formula(const std::optional<std::string>& value) {
    game_pad_stick_x_formula_ = value;

    coordinate_between_properties();
  }

  const std::optional<std::string>& get_game_pad_stick_y_formula(void) const {
    return game_pad_stick_y_formula_;
  }
  void set_game_pad_stick_y_formula(const std::optional<std::string>& value) {
    game_pad_stick_y_formula_ = value;

    coordinate_between_properties();
  }

  const std::optional<std::string>& get_game_pad_stick_vertical_wheel_formula(void) const {
    return game_pad_stick_vertical_wheel_formula_;
  }
  void set_game_pad_stick_vertical_wheel_formula(const std::optional<std::string>& value) {
    game_pad_stick_vertical_wheel_formula_ = value;

    coordinate_between_properties();
  }

  const std::optional<std::string>& get_game_pad_stick_horizontal_wheel_formula(void) const {
    return game_pad_stick_horizontal_wheel_formula_;
  }
  void set_game_pad_stick_horizontal_wheel_formula(const std::optional<std::string>& value) {
    game_pad_stick_horizontal_wheel_formula_ = value;

    coordinate_between_properties();
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

  static bool validate_stick_formula(const std::string& formula) {
    auto value = krbn::exprtk_utility::eval(formula,
                                            {
                                                {"radian", 0.0},
                                                {"delta_magnitude", 0.1},
                                                {"absolute_magnitude", 0.5},
                                            });
    return !std::isnan(value);
  }

  template <typename T>
  T find_default_value(const T& value) {
    return helper_values_.find_default_value(value);
  }

private:
  static std::string unmarshal_formula(const nlohmann::json& json, const std::string& name) {
    if (json.is_string()) {
      return json.get<std::string>();

    } else if (json.is_array()) {
      std::stringstream ss;

      for (const auto& j : json) {
        if (!j.is_string()) {
          goto error;
        }

        ss << j.get<std::string>() << '\n';
      }

      auto s = ss.str();
      if (!s.empty()) {
        s.pop_back();
      }

      return s;
    }

  error:
    throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be array of string, or string, but is `{1}`",
                                                  name,
                                                  pqrs::json::dump_for_error_message(json)));
  }

  static nlohmann::json marshal_formula(const std::string& formula) {
    std::stringstream ss(formula);
    std::string line;
    std::vector<std::string> lines;

    while (std::getline(ss, line, '\n')) {
      lines.push_back(line);
    }

    if (lines.size() == 1) {
      return formula;
    }

    return lines;
  }

  void coordinate_between_properties(void) {
    // Set `disable_built_in_keyboard_if_exists_` false if `treat_as_built_in_keyboard_` is true.
    // If both settings are true, the device will always be disabled.
    // To avoid this situation, set `disable_built_in_keyboard_if_exists_` to false.
    if (treat_as_built_in_keyboard_ && disable_built_in_keyboard_if_exists_) {
      disable_built_in_keyboard_if_exists_ = false;
    }
  }

  nlohmann::json json_;
  device_identifiers identifiers_;
  bool ignore_;
  bool manipulate_caps_lock_led_;
  bool treat_as_built_in_keyboard_;
  bool disable_built_in_keyboard_if_exists_;
  bool mouse_flip_x_;
  bool mouse_flip_y_;
  bool mouse_flip_vertical_wheel_;
  bool mouse_flip_horizontal_wheel_;
  bool mouse_swap_xy_;
  bool mouse_swap_wheels_;
  bool mouse_discard_x_;
  bool mouse_discard_y_;
  bool mouse_discard_vertical_wheel_;
  bool mouse_discard_horizontal_wheel_;
  bool game_pad_swap_sticks_;
  double game_pad_xy_stick_continued_movement_absolute_magnitude_threshold_;
  int game_pad_xy_stick_continued_movement_interval_milliseconds_;
  int game_pad_xy_stick_flicking_input_window_milliseconds_;
  double game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold_;
  int game_pad_wheels_stick_continued_movement_interval_milliseconds_;
  int game_pad_wheels_stick_flicking_input_window_milliseconds_;
  std::optional<std::string> game_pad_stick_x_formula_;
  std::optional<std::string> game_pad_stick_y_formula_;
  std::optional<std::string> game_pad_stick_vertical_wheel_formula_;
  std::optional<std::string> game_pad_stick_horizontal_wheel_formula_;
  simple_modifications simple_modifications_;
  simple_modifications fn_function_keys_;
  configuration_json_helper::helper_values helper_values_;
};

inline void to_json(nlohmann::json& json, const device& device) {
  json = device.to_json();
}
} // namespace details
} // namespace core_configuration
} // namespace krbn
