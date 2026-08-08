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
