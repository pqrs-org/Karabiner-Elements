#pragma once

#include "core_configuration/core_configuration.hpp"
#include <nlohmann/json.hpp>

class settings_configuration_updater final {
public:
  [[nodiscard]] static bool apply(const nlohmann::json& json,
                                  krbn::core_configuration::core_configuration& core_configuration) {
    auto changed = false;

    if (const auto it = json.find("global_configuration"); it != json.end()) {
      auto& global = core_configuration.get_global_configuration();
      const auto& global_json = *it;
      changed |= apply_value<bool>(global_json,
                                   "check_for_updates",
                                   global.get_check_for_updates(),
                                   [&](auto value) { global.set_check_for_updates(value); });
      changed |= apply_value<bool>(global_json,
                                   "show_in_menu_bar",
                                   global.get_show_in_menu_bar(),
                                   [&](auto value) { global.set_show_in_menu_bar(value); });
      changed |= apply_value<bool>(global_json,
                                   "show_profile_name_in_menu_bar",
                                   global.get_show_profile_name_in_menu_bar(),
                                   [&](auto value) { global.set_show_profile_name_in_menu_bar(value); });
      changed |= apply_value<bool>(global_json,
                                   "show_additional_menu_items",
                                   global.get_show_additional_menu_items(),
                                   [&](auto value) { global.set_show_additional_menu_items(value); });
      changed |= apply_value<bool>(global_json,
                                   "enable_notification_window",
                                   global.get_enable_notification_window(),
                                   [&](auto value) { global.set_enable_notification_window(value); });
      changed |= apply_value<bool>(global_json,
                                   "unsafe_ui",
                                   global.get_unsafe_ui(),
                                   [&](auto value) { global.set_unsafe_ui(value); });
      changed |= apply_value<bool>(global_json,
                                   "filter_useless_events_from_specific_devices",
                                   global.get_filter_useless_events_from_specific_devices(),
                                   [&](auto value) { global.set_filter_useless_events_from_specific_devices(value); });
      changed |= apply_value<bool>(global_json,
                                   "reorder_same_timestamp_input_events_to_prioritize_modifiers",
                                   global.get_reorder_same_timestamp_input_events_to_prioritize_modifiers(),
                                   [&](auto value) { global.set_reorder_same_timestamp_input_events_to_prioritize_modifiers(value); });
      changed |= apply_value<bool>(global_json,
                                   "enable_cgeventtap_fallback",
                                   global.get_enable_cgeventtap_fallback(),
                                   [&](auto value) { global.set_enable_cgeventtap_fallback(value); });
    }

    if (const auto it = json.find("machine_specific"); it != json.end()) {
      auto& machine_specific = core_configuration.get_machine_specific().get_entry();
      const auto& machine_specific_json = *it;
      changed |= apply_value<bool>(machine_specific_json,
                                   "enable_multitouch_extension",
                                   machine_specific.get_enable_multitouch_extension(),
                                   [&](auto value) { machine_specific.set_enable_multitouch_extension(value); });
      changed |= apply_value<std::string>(machine_specific_json,
                                          "external_editor_path",
                                          machine_specific.get_external_editor_path(),
                                          [&](const auto& value) { machine_specific.set_external_editor_path(value); });
    }

    if (const auto it = json.find("selected_profile"); it != json.end()) {
      auto& selected_profile = core_configuration.get_selected_profile();
      const auto& selected_profile_json = *it;

      changed |= apply_value<bool>(selected_profile_json,
                                   "modify_mouse_events_by_default",
                                   selected_profile.get_modify_mouse_events_by_default(),
                                   [&](auto value) { selected_profile.set_modify_mouse_events_by_default(value); });

      if (const auto parameters_it = selected_profile_json.find("parameters");
          parameters_it != selected_profile_json.end()) {
        auto parameters = selected_profile.get_parameters();
        const auto& parameters_json = *parameters_it;
        changed |= apply_value<int>(parameters_json,
                                    "delay_milliseconds_before_open_device",
                                    static_cast<int>(parameters->get_delay_milliseconds_before_open_device().count()),
                                    [&](auto value) {
                                      parameters->set_delay_milliseconds_before_open_device(
                                          std::chrono::milliseconds(value));
                                    });
      }

      if (const auto devices_it = selected_profile_json.find("devices");
          devices_it != selected_profile_json.end()) {
        for (const auto& [identifiers_json_string, device_json] : devices_it->items()) {
          auto identifiers = nlohmann::json::parse(identifiers_json_string).get<krbn::device_identifiers>();
          auto device = selected_profile.get_device(identifiers);

          changed |= apply_value<bool>(device_json, "ignore", device->get_ignore(), [&](auto value) { device->set_ignore(value); });
          changed |= apply_value<bool>(device_json, "manipulate_caps_lock_led", device->get_manipulate_caps_lock_led(), [&](auto value) { device->set_manipulate_caps_lock_led(value); });
          changed |= apply_value<bool>(device_json, "ignore_vendor_events", device->get_ignore_vendor_events(), [&](auto value) { device->set_ignore_vendor_events(value); });
          changed |= apply_value<bool>(device_json, "treat_as_built_in_keyboard", device->get_treat_as_built_in_keyboard(), [&](auto value) { device->set_treat_as_built_in_keyboard(value); });
          changed |= apply_value<bool>(device_json, "disable_built_in_keyboard_if_exists", device->get_disable_built_in_keyboard_if_exists(), [&](auto value) { device->set_disable_built_in_keyboard_if_exists(value); });
          changed |= apply_value<double>(device_json, "pointing_motion_xy_multiplier", device->get_pointing_motion_xy_multiplier(), [&](auto value) { device->set_pointing_motion_xy_multiplier(value); });
          changed |= apply_value<double>(device_json, "pointing_motion_wheels_multiplier", device->get_pointing_motion_wheels_multiplier(), [&](auto value) { device->set_pointing_motion_wheels_multiplier(value); });
          changed |= apply_value<bool>(device_json, "mouse_flip_x", device->get_mouse_flip_x(), [&](auto value) { device->set_mouse_flip_x(value); });
          changed |= apply_value<bool>(device_json, "mouse_flip_y", device->get_mouse_flip_y(), [&](auto value) { device->set_mouse_flip_y(value); });
          changed |= apply_value<bool>(device_json, "mouse_flip_vertical_wheel", device->get_mouse_flip_vertical_wheel(), [&](auto value) { device->set_mouse_flip_vertical_wheel(value); });
          changed |= apply_value<bool>(device_json, "mouse_flip_horizontal_wheel", device->get_mouse_flip_horizontal_wheel(), [&](auto value) { device->set_mouse_flip_horizontal_wheel(value); });
          changed |= apply_value<bool>(device_json, "mouse_discard_x", device->get_mouse_discard_x(), [&](auto value) { device->set_mouse_discard_x(value); });
          changed |= apply_value<bool>(device_json, "mouse_discard_y", device->get_mouse_discard_y(), [&](auto value) { device->set_mouse_discard_y(value); });
          changed |= apply_value<bool>(device_json, "mouse_discard_vertical_wheel", device->get_mouse_discard_vertical_wheel(), [&](auto value) { device->set_mouse_discard_vertical_wheel(value); });
          changed |= apply_value<bool>(device_json, "mouse_discard_horizontal_wheel", device->get_mouse_discard_horizontal_wheel(), [&](auto value) { device->set_mouse_discard_horizontal_wheel(value); });
          changed |= apply_value<bool>(device_json, "mouse_swap_xy", device->get_mouse_swap_xy(), [&](auto value) { device->set_mouse_swap_xy(value); });
          changed |= apply_value<bool>(device_json, "mouse_swap_wheels", device->get_mouse_swap_wheels(), [&](auto value) { device->set_mouse_swap_wheels(value); });
          changed |= apply_value<bool>(device_json, "game_pad_swap_sticks", device->get_game_pad_swap_sticks(), [&](auto value) { device->set_game_pad_swap_sticks(value); });
          changed |= apply_value<double>(device_json, "game_pad_xy_stick_deadzone", device->get_game_pad_xy_stick_deadzone(), [&](auto value) { device->set_game_pad_xy_stick_deadzone(value); });
          changed |= apply_value<double>(device_json, "game_pad_xy_stick_delta_magnitude_detection_threshold", device->get_game_pad_xy_stick_delta_magnitude_detection_threshold(), [&](auto value) { device->set_game_pad_xy_stick_delta_magnitude_detection_threshold(value); });
          changed |= apply_value<double>(device_json, "game_pad_xy_stick_continued_movement_absolute_magnitude_threshold", device->get_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(), [&](auto value) { device->set_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(value); });
          changed |= apply_value<int>(device_json, "game_pad_xy_stick_continued_movement_interval_milliseconds", device->get_game_pad_xy_stick_continued_movement_interval_milliseconds(), [&](auto value) { device->set_game_pad_xy_stick_continued_movement_interval_milliseconds(value); });
          changed |= apply_value<double>(device_json, "game_pad_wheels_stick_deadzone", device->get_game_pad_wheels_stick_deadzone(), [&](auto value) { device->set_game_pad_wheels_stick_deadzone(value); });
          changed |= apply_value<double>(device_json, "game_pad_wheels_stick_delta_magnitude_detection_threshold", device->get_game_pad_wheels_stick_delta_magnitude_detection_threshold(), [&](auto value) { device->set_game_pad_wheels_stick_delta_magnitude_detection_threshold(value); });
          changed |= apply_value<double>(device_json, "game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold", device->get_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(), [&](auto value) { device->set_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(value); });
          changed |= apply_value<int>(device_json, "game_pad_wheels_stick_continued_movement_interval_milliseconds", device->get_game_pad_wheels_stick_continued_movement_interval_milliseconds(), [&](auto value) { device->set_game_pad_wheels_stick_continued_movement_interval_milliseconds(value); });
        }
      }

      if (const auto complex_modifications_it = selected_profile_json.find("complex_modifications");
          complex_modifications_it != selected_profile_json.end()) {
        if (const auto parameters_it = complex_modifications_it->find("parameters");
            parameters_it != complex_modifications_it->end()) {
          auto complex_parameters = selected_profile.get_complex_modifications()->get_parameters();
          const auto& complex_parameters_json = *parameters_it;
          changed |= apply_value<int>(complex_parameters_json,
                                      "basic_simultaneous_threshold_milliseconds",
                                      complex_parameters->get_basic_simultaneous_threshold_milliseconds(),
                                      [&](auto value) { complex_parameters->set_basic_simultaneous_threshold_milliseconds(value); });
          changed |= apply_value<int>(complex_parameters_json,
                                      "basic_to_if_alone_timeout_milliseconds",
                                      complex_parameters->get_basic_to_if_alone_timeout_milliseconds(),
                                      [&](auto value) { complex_parameters->set_basic_to_if_alone_timeout_milliseconds(value); });
          changed |= apply_value<int>(complex_parameters_json,
                                      "basic_to_if_held_down_threshold_milliseconds",
                                      complex_parameters->get_basic_to_if_held_down_threshold_milliseconds(),
                                      [&](auto value) { complex_parameters->set_basic_to_if_held_down_threshold_milliseconds(value); });
          changed |= apply_value<int>(complex_parameters_json,
                                      "basic_to_delayed_action_delay_milliseconds",
                                      complex_parameters->get_basic_to_delayed_action_delay_milliseconds(),
                                      [&](auto value) { complex_parameters->set_basic_to_delayed_action_delay_milliseconds(value); });
          changed |= apply_value<int>(complex_parameters_json,
                                      "mouse_motion_to_scroll_speed",
                                      complex_parameters->get_mouse_motion_to_scroll_speed(),
                                      [&](auto value) { complex_parameters->set_mouse_motion_to_scroll_speed(value); });
        }
      }

      if (const auto virtual_hid_keyboard_it = selected_profile_json.find("virtual_hid_keyboard");
          virtual_hid_keyboard_it != selected_profile_json.end()) {
        auto virtual_hid_keyboard = selected_profile.get_virtual_hid_keyboard();
        const auto& virtual_hid_keyboard_json = *virtual_hid_keyboard_it;
        changed |= apply_value<std::string>(virtual_hid_keyboard_json,
                                            "keyboard_type_v2",
                                            virtual_hid_keyboard->get_keyboard_type_v2(),
                                            [&](const auto& value) { virtual_hid_keyboard->set_keyboard_type_v2(value); });
        changed |= apply_value<int>(virtual_hid_keyboard_json,
                                    "mouse_key_xy_scale",
                                    virtual_hid_keyboard->get_mouse_key_xy_scale(),
                                    [&](auto value) { virtual_hid_keyboard->set_mouse_key_xy_scale(value); });
        changed |= apply_value<bool>(virtual_hid_keyboard_json,
                                     "indicate_sticky_modifier_keys_state",
                                     virtual_hid_keyboard->get_indicate_sticky_modifier_keys_state(),
                                     [&](auto value) { virtual_hid_keyboard->set_indicate_sticky_modifier_keys_state(value); });
      }
    }

    return changed;
  }

private:
  template <typename T, typename Setter>
  [[nodiscard]] static bool apply_value(const nlohmann::json& json,
                                        const char* key,
                                        const T& current_value,
                                        Setter&& setter) {
    const auto it = json.find(key);
    if (it == json.end()) {
      return false;
    }

    const auto value = it->get<T>();
    if (value == current_value) {
      return false;
    }

    setter(value);
    return true;
  }
};
