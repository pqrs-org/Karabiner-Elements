#pragma once

#include "core_configuration/core_configuration.hpp"
#include "settings_remembered_device_identifiers.hpp"
#include <nlohmann/json.hpp>

class settings_configuration_snapshot final {
public:
  explicit settings_configuration_snapshot(const krbn::core_configuration::core_configuration& core_configuration)
      : core_configuration_(core_configuration) {
  }

  [[nodiscard]] nlohmann::json to_json() const {
    const auto& global = core_configuration_.get_global_configuration();
    const auto& machine_specific = core_configuration_.get_machine_specific().get_entry();
    const auto& selected_profile = core_configuration_.get_selected_profile();
    const auto& complex_modifications = selected_profile.get_complex_modifications();
    const auto& complex_parameters = complex_modifications->get_parameters();
    const auto& virtual_hid_keyboard = selected_profile.get_virtual_hid_keyboard();

    auto profiles = nlohmann::json::array();
    const auto& source_profiles = core_configuration_.get_profiles();
    for (size_t index = 0; index < source_profiles.size(); ++index) {
      profiles.push_back({
          {"index", index},
          {"name", source_profiles[index]->get_name()},
          {"selected", source_profiles[index]->get_selected()},
      });
    }

    return {
        {"device_defaults", make_device_defaults_json()},
        {"global_configuration",
         {
             {"check_for_updates", global.get_check_for_updates()},
             {"show_in_menu_bar", global.get_show_in_menu_bar()},
             {"show_profile_name_in_menu_bar", global.get_show_profile_name_in_menu_bar()},
             {"show_additional_menu_items", global.get_show_additional_menu_items()},
             {"enable_notification_window", global.get_enable_notification_window()},
             {"unsafe_ui", global.get_unsafe_ui()},
             {"filter_useless_events_from_specific_devices", global.get_filter_useless_events_from_specific_devices()},
             {"reorder_same_timestamp_input_events_to_prioritize_modifiers", global.get_reorder_same_timestamp_input_events_to_prioritize_modifiers()},
             {"enable_cgeventtap_fallback", global.get_enable_cgeventtap_fallback()},
         }},
        {"machine_specific",
         {
             {"enable_multitouch_extension", machine_specific.get_enable_multitouch_extension()},
             {"external_editor_path", machine_specific.get_external_editor_path()},
         }},
        {"profiles", profiles},
        {"selected_profile",
         {
             {"parameters",
              {
                  {"delay_milliseconds_before_open_device", selected_profile.get_parameters()->get_delay_milliseconds_before_open_device().count()},
              }},
             {"simple_modifications", make_simple_modifications_json(*selected_profile.get_simple_modifications())},
             {"fn_function_keys", make_simple_modifications_json(*selected_profile.get_fn_function_keys())},
             {"devices", make_devices_json(selected_profile)},
             {"complex_modifications",
              {
                  {"rules", make_complex_modifications_rules_json(complex_modifications->get_rules())},
                  {"parameters",
                   {
                       {"basic_simultaneous_threshold_milliseconds", complex_parameters->get_basic_simultaneous_threshold_milliseconds()},
                       {"basic_to_if_alone_timeout_milliseconds", complex_parameters->get_basic_to_if_alone_timeout_milliseconds()},
                       {"basic_to_if_held_down_threshold_milliseconds", complex_parameters->get_basic_to_if_held_down_threshold_milliseconds()},
                       {"basic_to_delayed_action_delay_milliseconds", complex_parameters->get_basic_to_delayed_action_delay_milliseconds()},
                       {"mouse_motion_to_scroll_speed", complex_parameters->get_mouse_motion_to_scroll_speed()},
                   }},
              }},
             {"virtual_hid_keyboard",
              {
                  {"keyboard_type_v2", virtual_hid_keyboard->get_keyboard_type_v2()},
                  {"mouse_key_xy_scale", virtual_hid_keyboard->get_mouse_key_xy_scale()},
                  {"indicate_sticky_modifier_keys_state", virtual_hid_keyboard->get_indicate_sticky_modifier_keys_state()},
              }},
         }},
    };
  }

private:
  [[nodiscard]] static nlohmann::json make_device_defaults_json() {
    krbn::core_configuration::details::device device;

    return {
        {"pointing_motion_xy_multiplier", device.find_default_value(device.get_pointing_motion_xy_multiplier())},
        {"pointing_motion_wheels_multiplier", device.find_default_value(device.get_pointing_motion_wheels_multiplier())},
        {"game_pad_xy_stick_deadzone", device.find_default_value(device.get_game_pad_xy_stick_deadzone())},
        {"game_pad_xy_stick_delta_magnitude_detection_threshold", device.find_default_value(device.get_game_pad_xy_stick_delta_magnitude_detection_threshold())},
        {"game_pad_xy_stick_continued_movement_absolute_magnitude_threshold", device.find_default_value(device.get_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold())},
        {"game_pad_xy_stick_continued_movement_interval_milliseconds", device.find_default_value(device.get_game_pad_xy_stick_continued_movement_interval_milliseconds())},
        {"game_pad_wheels_stick_deadzone", device.find_default_value(device.get_game_pad_wheels_stick_deadzone())},
        {"game_pad_wheels_stick_delta_magnitude_detection_threshold", device.find_default_value(device.get_game_pad_wheels_stick_delta_magnitude_detection_threshold())},
        {"game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold", device.find_default_value(device.get_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold())},
        {"game_pad_wheels_stick_continued_movement_interval_milliseconds", device.find_default_value(device.get_game_pad_wheels_stick_continued_movement_interval_milliseconds())},
    };
  }

  [[nodiscard]] static nlohmann::json make_simple_modifications_json(const krbn::core_configuration::details::simple_modifications& simple_modifications) {
    auto json = nlohmann::json::array();
    const auto& pairs = simple_modifications.get_pairs();
    for (size_t index = 0; index < pairs.size(); ++index) {
      json.push_back({
          {"index", index},
          {"from_json_string", pairs[index].first},
          {"to_json_string", pairs[index].second},
      });
    }
    return json;
  }

  [[nodiscard]] static nlohmann::json make_complex_modifications_rules_json(const std::vector<pqrs::not_null_shared_ptr_t<krbn::core_configuration::details::complex_modifications_rule>>& rules) {
    auto json = nlohmann::json::array();
    for (size_t index = 0; index < rules.size(); ++index) {
      const auto& rule = rules[index];
      json.push_back({
          {"index", index},
          {"description", rule->get_description()},
          {"enabled", rule->get_enabled()},
          {"code_string", rule->get_code_string()},
          {"search_text", rule->get_search_text()},
          {"code_type", rule->get_code_type() == krbn::core_configuration::details::complex_modifications_rule::code_type::javascript ? "javascript" : "json"},
      });
    }
    return json;
  }

  [[nodiscard]] nlohmann::json make_devices_json(const krbn::core_configuration::details::profile& profile) const {
    auto identifiers = settings_remembered_device_identifiers::get_instance().get_device_identifiers();

    for (const auto& device : profile.get_devices()) {
      if (!std::ranges::contains(identifiers, device->get_identifiers())) {
        identifiers.push_back(device->get_identifiers());
      }
    }

    auto json = nlohmann::json::object();
    for (const auto& i : identifiers) {
      const auto& device = profile.get_device(i);
      json[i.to_normalized_json().dump()] = {
          {"ignore", device->get_ignore()},
          {"manipulate_caps_lock_led", device->get_manipulate_caps_lock_led()},
          {"ignore_vendor_events", device->get_ignore_vendor_events()},
          {"treat_as_built_in_keyboard", device->get_treat_as_built_in_keyboard()},
          {"disable_built_in_keyboard_if_exists", device->get_disable_built_in_keyboard_if_exists()},
          {"pointing_motion_xy_multiplier", device->get_pointing_motion_xy_multiplier()},
          {"pointing_motion_wheels_multiplier", device->get_pointing_motion_wheels_multiplier()},
          {"mouse_flip_x", device->get_mouse_flip_x()},
          {"mouse_flip_y", device->get_mouse_flip_y()},
          {"mouse_flip_vertical_wheel", device->get_mouse_flip_vertical_wheel()},
          {"mouse_flip_horizontal_wheel", device->get_mouse_flip_horizontal_wheel()},
          {"mouse_discard_x", device->get_mouse_discard_x()},
          {"mouse_discard_y", device->get_mouse_discard_y()},
          {"mouse_discard_vertical_wheel", device->get_mouse_discard_vertical_wheel()},
          {"mouse_discard_horizontal_wheel", device->get_mouse_discard_horizontal_wheel()},
          {"mouse_swap_xy", device->get_mouse_swap_xy()},
          {"mouse_swap_wheels", device->get_mouse_swap_wheels()},
          {"game_pad_swap_sticks", device->get_game_pad_swap_sticks()},
          {"game_pad_xy_stick_deadzone", device->get_game_pad_xy_stick_deadzone()},
          {"game_pad_xy_stick_delta_magnitude_detection_threshold", device->get_game_pad_xy_stick_delta_magnitude_detection_threshold()},
          {"game_pad_xy_stick_continued_movement_absolute_magnitude_threshold", device->get_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold()},
          {"game_pad_xy_stick_continued_movement_interval_milliseconds", device->get_game_pad_xy_stick_continued_movement_interval_milliseconds()},
          {"game_pad_wheels_stick_deadzone", device->get_game_pad_wheels_stick_deadzone()},
          {"game_pad_wheels_stick_delta_magnitude_detection_threshold", device->get_game_pad_wheels_stick_delta_magnitude_detection_threshold()},
          {"game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold", device->get_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold()},
          {"game_pad_wheels_stick_continued_movement_interval_milliseconds", device->get_game_pad_wheels_stick_continued_movement_interval_milliseconds()},
          {"game_pad_stick_x_formula", device->get_game_pad_stick_x_formula()},
          {"game_pad_stick_y_formula", device->get_game_pad_stick_y_formula()},
          {"game_pad_stick_vertical_wheel_formula", device->get_game_pad_stick_vertical_wheel_formula()},
          {"game_pad_stick_horizontal_wheel_formula", device->get_game_pad_stick_horizontal_wheel_formula()},
          {"simple_modifications", make_simple_modifications_json(*device->get_simple_modifications())},
          {"fn_function_keys", make_simple_modifications_json(*device->get_fn_function_keys())},
      };
    }

    return json;
  }

  const krbn::core_configuration::core_configuration& core_configuration_;
};
