#pragma once

#include "../../configuration_json_helper.hpp"
#include "exprtk_utility.hpp"
#include "simple_modifications.hpp"
#include <functional>
#include <pqrs/string.hpp>
#include <ranges>
#include <string_view>

namespace krbn::core_configuration::details {
class device final {
public:
  // Some default values depend on the device identifiers. For example, `ignore` is
  // false for ordinary keyboards, but true for game pads. These values are resolved
  // before loading the JSON values so that omitted values use the device-specific
  // defaults and only explicit overrides are serialized.
  struct default_values final {
    bool ignore;
    bool manipulate_caps_lock_led;
  };

  using default_values_resolver = std::function<default_values(const device_identifiers&)>;

  device(const device&) = delete;

  device()
      : device(nlohmann::json::object(),
               krbn::core_configuration::error_handling::loose) {
  }

  device(const nlohmann::json& json,
         error_handling error_handling)
      : device(json,
               error_handling,
               [](const auto&) {
                 return default_values{
                     .ignore = false,
                     .manipulate_caps_lock_led = false,
                 };
               }) {
  }

  device(const nlohmann::json& json,
         error_handling error_handling,
         const default_values_resolver& resolve_default_values)
      : json_(json),
        identifiers_(make_device_identifiers(json)),
        ignore_(false),
        ignore_configured_(false),
        simple_modifications_(std::make_shared<simple_modifications>()),
        fn_function_keys_(std::make_shared<simple_modifications>()) {
    const auto default_values = resolve_default_values(identifiers_);

    ignore_ = default_values.ignore;

    helper_values_.push_back_value<bool>("manipulate_caps_lock_led",
                                         manipulate_caps_lock_led_,
                                         default_values.manipulate_caps_lock_led);

    helper_values_.push_back_value<bool>("swap_grave_accent_and_non_us_backslash",
                                         swap_grave_accent_and_non_us_backslash_,
                                         false);

    helper_values_.push_back_value<bool>("ignore_vendor_events",
                                         ignore_vendor_events_,
                                         false);

    helper_values_.push_back_value<bool>("treat_as_built_in_keyboard",
                                         treat_as_built_in_keyboard_,
                                         false);

    helper_values_.push_back_value<bool>("disable_built_in_keyboard_if_exists",
                                         disable_built_in_keyboard_if_exists_,
                                         false);

    helper_values_.push_back_value<double>("pointing_motion_xy_multiplier",
                                           pointing_motion_xy_multiplier_,
                                           1.0);

    helper_values_.push_back_value<double>("pointing_motion_wheels_multiplier",
                                           pointing_motion_wheels_multiplier_,
                                           1.0);

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

    helper_values_.push_back_value<double>("game_pad_xy_stick_deadzone",
                                           game_pad_xy_stick_deadzone_,
                                           0.1);

    helper_values_.push_back_value<double>("game_pad_xy_stick_delta_magnitude_detection_threshold",
                                           game_pad_xy_stick_delta_magnitude_detection_threshold_,
                                           0.02);

    helper_values_.push_back_value<double>("game_pad_xy_stick_continued_movement_absolute_magnitude_threshold",
                                           game_pad_xy_stick_continued_movement_absolute_magnitude_threshold_,
                                           1.0);

    helper_values_.push_back_value<int>("game_pad_xy_stick_continued_movement_interval_milliseconds",
                                        game_pad_xy_stick_continued_movement_interval_milliseconds_,
                                        20);

    helper_values_.push_back_value<double>("game_pad_wheels_stick_deadzone",
                                           game_pad_wheels_stick_deadzone_,
                                           0.1);

    helper_values_.push_back_value<double>("game_pad_wheels_stick_delta_magnitude_detection_threshold",
                                           game_pad_wheels_stick_delta_magnitude_detection_threshold_,
                                           0.02);

    helper_values_.push_back_value<double>("game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold",
                                           game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold_,
                                           1.0);

    helper_values_.push_back_value<int>("game_pad_wheels_stick_continued_movement_interval_milliseconds",
                                        game_pad_wheels_stick_continued_movement_interval_milliseconds_,
                                        10);

    helper_values_.push_back_value<std::string>("game_pad_stick_x_formula",
                                                game_pad_stick_x_formula_,
                                                // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
                                                pqrs::string::trim_copy(R"(

var m:= 0;

if (continued_movement == false) {
  m := delta_magnitude * 16;
} else if (absolute_magnitude < 1.5) {
  m := absolute_magnitude * 8;
} else if (absolute_magnitude < 2) {
  m := absolute_magnitude * 12;
} else {
  m := absolute_magnitude * 24;
};

cos(radian) * m;

)"));

    helper_values_.push_back_value<std::string>("game_pad_stick_y_formula",
                                                game_pad_stick_y_formula_,
                                                // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
                                                pqrs::string::trim_copy(R"(

var m:= 0;

if (continued_movement == false) {
  m := delta_magnitude * 16;
} else if (absolute_magnitude < 1.5) {
  m := absolute_magnitude * 8;
} else if (absolute_magnitude < 2) {
  m := absolute_magnitude * 12;
} else {
  m := absolute_magnitude * 24;
};

sin(radian) * m;

)"));

    helper_values_.push_back_value<std::string>("game_pad_stick_vertical_wheel_formula",
                                                game_pad_stick_vertical_wheel_formula_,
                                                // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
                                                pqrs::string::trim_copy(R"(

var m := 0;

if (abs(cos(radian)) < abs(sin(radian))) {
  if (continued_movement == false) {
    m := delta_magnitude;
  } else {
    m := absolute_magnitude * 0.1;
  };
};

sin(radian) * m;

)"));

    helper_values_.push_back_value<std::string>("game_pad_stick_horizontal_wheel_formula",
                                                game_pad_stick_horizontal_wheel_formula_,
                                                // The logical value range of Karabiner-DriverKit-VirtualHIDPointing is -127 ... 127.
                                                pqrs::string::trim_copy(R"(

var m := 0;

if (abs(cos(radian)) > abs(sin(radian))) {
  if (continued_movement == false) {
    m := delta_magnitude;
  } else {
    m := absolute_magnitude * 0.1;
  };
};

cos(radian) * m;

)"));

    //
    // Set default value
    //

    fn_function_keys_->update(make_default_fn_function_keys_json());

    //
    // Load from json
    //

    pqrs::json::requires_object(json, "json");

    helper_values_.update_value(json, error_handling);

    if (auto it = json.find("ignore");
        it != std::end(json)) {
      try {
        pqrs::json::requires_boolean(*it, "`ignore`");
        ignore_ = it->get<bool>();
        ignore_configured_ = true;
      } catch (const std::exception& e) {
        if (error_handling == error_handling::strict) {
          throw;
        } else {
          logger::get_logger()->warn(e.what());
        }
      }
    }

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
      }
    }

    //
    // Coordinate between settings.
    //

    coordinate_between_properties();
  }

  static nlohmann::json make_default_fn_function_keys_json() {
    auto json = nlohmann::json::array();

    for (int i = 1; i <= 12; ++i) {
      json.push_back(nlohmann::json::object({
          {"from", nlohmann::json::object({{"key_code", fmt::format("f{0}", i)}})},
          {"to", nlohmann::json::object()},
      }));
    }

    return json;
  }

  nlohmann::json to_json() const {
    auto j = json_;

    helper_values_.update_json(j);

    if (ignore_configured_) {
      j["ignore"] = ignore_;
    } else {
      j.erase("ignore");
    }

    j["identifiers"] = identifiers_;

    {
      auto jj = simple_modifications_->to_json(nlohmann::json::array());
      if (!jj.empty()) {
        j["simple_modifications"] = jj;
      } else {
        j.erase("simple_modifications");
      }
    }

    {
      auto jj = fn_function_keys_->to_json(make_default_fn_function_keys_json());
      if (!jj.empty()) {
        j["fn_function_keys"] = jj;
      } else {
        j.erase("fn_function_keys");
      }
    }

    //
    // Add `identifiers` only if it contains some settings.
    //

    if (j.size() == 1) {
      j.erase("identifiers");
    }

    return j;
  }

  [[nodiscard]] const device_identifiers& get_identifiers() const {
    return identifiers_;
  }

  [[nodiscard]] const bool& get_ignore() const {
    return ignore_;
  }
  [[nodiscard]] bool get_ignore_configured() const {
    return ignore_configured_;
  }
  void set_ignore(bool value) {
    ignore_ = value;
    ignore_configured_ = true;

    coordinate_between_properties();
  }

  void update_default_values(const default_values& values) {
    if (!ignore_configured_) {
      ignore_ = values.ignore;
    }

    helper_values_.set_default_value(manipulate_caps_lock_led_,
                                     values.manipulate_caps_lock_led);

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_manipulate_caps_lock_led() const {
    return manipulate_caps_lock_led_;
  }
  void set_manipulate_caps_lock_led(bool value) {
    manipulate_caps_lock_led_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_swap_grave_accent_and_non_us_backslash() const {
    return swap_grave_accent_and_non_us_backslash_;
  }
  void set_swap_grave_accent_and_non_us_backslash(bool value) {
    swap_grave_accent_and_non_us_backslash_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_ignore_vendor_events() const {
    return ignore_vendor_events_;
  }
  void set_ignore_vendor_events(bool value) {
    ignore_vendor_events_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_treat_as_built_in_keyboard() const {
    return treat_as_built_in_keyboard_;
  }
  void set_treat_as_built_in_keyboard(bool value) {
    treat_as_built_in_keyboard_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_disable_built_in_keyboard_if_exists() const {
    return disable_built_in_keyboard_if_exists_;
  }
  void set_disable_built_in_keyboard_if_exists(bool value) {
    disable_built_in_keyboard_if_exists_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const double& get_pointing_motion_xy_multiplier() const {
    return pointing_motion_xy_multiplier_;
  }
  void set_pointing_motion_xy_multiplier(double value) {
    pointing_motion_xy_multiplier_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const double& get_pointing_motion_wheels_multiplier() const {
    return pointing_motion_wheels_multiplier_;
  }
  void set_pointing_motion_wheels_multiplier(double value) {
    pointing_motion_wheels_multiplier_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_mouse_flip_x() const {
    return mouse_flip_x_;
  }
  void set_mouse_flip_x(bool value) {
    mouse_flip_x_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_mouse_flip_y() const {
    return mouse_flip_y_;
  }
  void set_mouse_flip_y(bool value) {
    mouse_flip_y_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_mouse_flip_vertical_wheel() const {
    return mouse_flip_vertical_wheel_;
  }
  void set_mouse_flip_vertical_wheel(bool value) {
    mouse_flip_vertical_wheel_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_mouse_flip_horizontal_wheel() const {
    return mouse_flip_horizontal_wheel_;
  }
  void set_mouse_flip_horizontal_wheel(bool value) {
    mouse_flip_horizontal_wheel_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_mouse_swap_xy() const {
    return mouse_swap_xy_;
  }
  void set_mouse_swap_xy(bool value) {
    mouse_swap_xy_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_mouse_swap_wheels() const {
    return mouse_swap_wheels_;
  }
  void set_mouse_swap_wheels(bool value) {
    mouse_swap_wheels_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_mouse_discard_x() const {
    return mouse_discard_x_;
  }
  void set_mouse_discard_x(bool value) {
    mouse_discard_x_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_mouse_discard_y() const {
    return mouse_discard_y_;
  }
  void set_mouse_discard_y(bool value) {
    mouse_discard_y_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_mouse_discard_vertical_wheel() const {
    return mouse_discard_vertical_wheel_;
  }
  void set_mouse_discard_vertical_wheel(bool value) {
    mouse_discard_vertical_wheel_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_mouse_discard_horizontal_wheel() const {
    return mouse_discard_horizontal_wheel_;
  }
  void set_mouse_discard_horizontal_wheel(bool value) {
    mouse_discard_horizontal_wheel_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const bool& get_game_pad_swap_sticks() const {
    return game_pad_swap_sticks_;
  }
  void set_game_pad_swap_sticks(bool value) {
    game_pad_swap_sticks_ = value;

    coordinate_between_properties();
  }

  //
  // game_pad_xy_stick_XXX
  //

  [[nodiscard]] const double& get_game_pad_xy_stick_deadzone() const {
    return game_pad_xy_stick_deadzone_;
  }
  void set_game_pad_xy_stick_deadzone(double value) {
    game_pad_xy_stick_deadzone_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const double& get_game_pad_xy_stick_delta_magnitude_detection_threshold() const {
    return game_pad_xy_stick_delta_magnitude_detection_threshold_;
  }
  void set_game_pad_xy_stick_delta_magnitude_detection_threshold(double value) {
    game_pad_xy_stick_delta_magnitude_detection_threshold_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const double& get_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold() const {
    return game_pad_xy_stick_continued_movement_absolute_magnitude_threshold_;
  }
  void set_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(double value) {
    game_pad_xy_stick_continued_movement_absolute_magnitude_threshold_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const int& get_game_pad_xy_stick_continued_movement_interval_milliseconds() const {
    return game_pad_xy_stick_continued_movement_interval_milliseconds_;
  }
  void set_game_pad_xy_stick_continued_movement_interval_milliseconds(int value) {
    game_pad_xy_stick_continued_movement_interval_milliseconds_ = value;

    coordinate_between_properties();
  }

  //
  // game_pad_wheels_stick_XXX
  //

  [[nodiscard]] const double& get_game_pad_wheels_stick_deadzone() const {
    return game_pad_wheels_stick_deadzone_;
  }
  void set_game_pad_wheels_stick_deadzone(double value) {
    game_pad_wheels_stick_deadzone_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const double& get_game_pad_wheels_stick_delta_magnitude_detection_threshold() const {
    return game_pad_wheels_stick_delta_magnitude_detection_threshold_;
  }
  void set_game_pad_wheels_stick_delta_magnitude_detection_threshold(double value) {
    game_pad_wheels_stick_delta_magnitude_detection_threshold_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const double& get_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold() const {
    return game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold_;
  }
  void set_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(double value) {
    game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const int& get_game_pad_wheels_stick_continued_movement_interval_milliseconds() const {
    return game_pad_wheels_stick_continued_movement_interval_milliseconds_;
  }
  void set_game_pad_wheels_stick_continued_movement_interval_milliseconds(int value) {
    game_pad_wheels_stick_continued_movement_interval_milliseconds_ = value;

    coordinate_between_properties();
  }

  [[nodiscard]] const std::string& get_game_pad_stick_x_formula() const {
    return game_pad_stick_x_formula_;
  }
  void set_game_pad_stick_x_formula(const std::string& value) {
    game_pad_stick_x_formula_ = pqrs::string::trim_copy(value);

    coordinate_between_properties();
  }

  [[nodiscard]] const std::string& get_game_pad_stick_y_formula() const {
    return game_pad_stick_y_formula_;
  }
  void set_game_pad_stick_y_formula(const std::string& value) {
    game_pad_stick_y_formula_ = pqrs::string::trim_copy(value);

    coordinate_between_properties();
  }

  [[nodiscard]] const std::string& get_game_pad_stick_vertical_wheel_formula() const {
    return game_pad_stick_vertical_wheel_formula_;
  }
  void set_game_pad_stick_vertical_wheel_formula(const std::string& value) {
    game_pad_stick_vertical_wheel_formula_ = pqrs::string::trim_copy(value);

    coordinate_between_properties();
  }

  [[nodiscard]] const std::string& get_game_pad_stick_horizontal_wheel_formula() const {
    return game_pad_stick_horizontal_wheel_formula_;
  }
  void set_game_pad_stick_horizontal_wheel_formula(const std::string& value) {
    game_pad_stick_horizontal_wheel_formula_ = pqrs::string::trim_copy(value);

    coordinate_between_properties();
  }

  [[nodiscard]] pqrs::not_null_shared_ptr_t<simple_modifications> get_simple_modifications() const {
    return simple_modifications_;
  }

  [[nodiscard]] pqrs::not_null_shared_ptr_t<simple_modifications> get_fn_function_keys() const {
    return fn_function_keys_;
  }

  [[nodiscard]] static bool validate_stick_formula(const std::string& formula) {
    auto expression = krbn::exprtk_utility::compile(formula);
    expression->set_variable("radian", 0.0);
    expression->set_variable("delta_magnitude", 0.1);
    expression->set_variable("absolute_magnitude", 0.5);
    expression->set_variable("continued_movement", 1.0);
    return !std::isnan(expression->value());
  }

  template <typename T>
  [[nodiscard]] T find_default_value(const T& value) {
    return helper_values_.find_default_value(value);
  }

private:
  [[nodiscard]] static device_identifiers make_device_identifiers(const nlohmann::json& json) {
    if (auto it = json.find("identifiers");
        it != std::end(json)) {
      try {
        return it->get<device_identifiers>();
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`identifiers` error: {0}", e.what()));
      }
    }

    return device_identifiers();
  }

  void coordinate_between_properties() {
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
  // The default value of `ignore_` can change at runtime, most notably when the
  // profile's `ignore_pointing_device_events_by_default` setting changes. An
  // explicitly configured value can then temporarily equal the new default. Keep
  // track of that explicitness and continue serializing the value; otherwise, a
  // subsequent profile default change would incorrectly treat it as inherited and
  // overwrite the user's device-specific choice.
  //
  // For example, consider a pointing device for which the user explicitly enables
  // event modification (`ignore_ == false`):
  //
  // ignore_pointing_device_events_by_default | configured | default | ignore_ | JSON
  // ----------------------------------------- | ---------- | ------- | ------- | -----
  // true                                      | false      | true    | true    | omitted
  // true                                      | true       | true    | false   | false
  // false                                     | true       | false   | false   | false
  // true                                      | true       | true    | false   | false
  //
  // Without `ignore_configured_`, the explicit `ignore_ == false` from the second
  // row would become indistinguishable from an inherited value in the third row,
  // where it equals the new default. The value would then be omitted from JSON and
  // changed back to true along with the default in the fourth row. Keeping it
  // configured preserves the user's device-specific choice throughout the change.
  bool ignore_configured_;
  bool manipulate_caps_lock_led_;
  // macOS maps these two HID usages differently depending on the keyboard's
  // device type. This setting compensates when the physical device and the
  // virtual keyboard require opposite mappings.
  // https://github.com/apple-oss-distributions/IOHIDFamily/blob/777ccd9698845aadf711e32d843c8c9b777431d9/IOHIDFamily/IOHIDKeyboard.cpp#L415-L432
  bool swap_grave_accent_and_non_us_backslash_;
  bool ignore_vendor_events_;
  bool treat_as_built_in_keyboard_;
  bool disable_built_in_keyboard_if_exists_;
  double pointing_motion_xy_multiplier_;
  double pointing_motion_wheels_multiplier_;
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

  double game_pad_xy_stick_deadzone_;
  double game_pad_xy_stick_delta_magnitude_detection_threshold_;
  double game_pad_xy_stick_continued_movement_absolute_magnitude_threshold_;
  int game_pad_xy_stick_continued_movement_interval_milliseconds_;

  double game_pad_wheels_stick_deadzone_;
  double game_pad_wheels_stick_delta_magnitude_detection_threshold_;
  double game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold_;
  int game_pad_wheels_stick_continued_movement_interval_milliseconds_;

  std::string game_pad_stick_x_formula_;
  std::string game_pad_stick_y_formula_;
  std::string game_pad_stick_vertical_wheel_formula_;
  std::string game_pad_stick_horizontal_wheel_formula_;
  pqrs::not_null_shared_ptr_t<simple_modifications> simple_modifications_;
  pqrs::not_null_shared_ptr_t<simple_modifications> fn_function_keys_;
  configuration_json_helper::helper_values helper_values_;
};

inline void to_json(nlohmann::json& json, const device& device) {
  json = device.to_json();
}
} // namespace krbn::core_configuration::details
