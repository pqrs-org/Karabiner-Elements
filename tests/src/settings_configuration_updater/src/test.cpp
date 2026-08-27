#include "../../../../src/apps/SettingsWindow/src/cpp/settings_configuration_updater.hpp"
#include "../../share/ut_helper.hpp"
#include <boost/ut.hpp>

namespace {
krbn::device_identifiers make_pointing_device_identifiers() {
  return krbn::device_identifiers(
      krbn::device_identifiers::initialization_parameters{
          .vendor_id = pqrs::hid::vendor_id::value_t(0x05ac),
          .product_id = pqrs::hid::product_id::value_t(1234),
          .is_pointing_device = true,
      });
}
} // namespace

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "settings_configuration_updater.apply_patch ignores empty and unknown patches"_test = [] {
    // Set up a default configuration.
    krbn::core_configuration::core_configuration core_configuration;

    // Verify that a completely empty patch reports no change.
    expect(!settings_configuration_updater::apply_patch(nlohmann::json::object(),
                                                        core_configuration));

    // Verify that an unknown top-level key reports no change.
    expect(!settings_configuration_updater::apply_patch(
        nlohmann::json({{"unknown", true}}),
        core_configuration));

    // Verify that a known section without values also reports no change.
    expect(!settings_configuration_updater::apply_patch(
        nlohmann::json({{"global_configuration", nlohmann::json::object()}}),
        core_configuration));
  };

  "settings_configuration_updater.apply_patch updates global and machine-specific settings"_test = [] {
    // Set up a default configuration.
    krbn::core_configuration::core_configuration core_configuration;

    // Build a patch that covers global settings, nested notification colors,
    // and machine-specific settings.
    auto patch = nlohmann::json::object();
    auto& global_patch = patch["global_configuration"];
    global_patch["check_for_updates"] = false;
    global_patch["show_in_menu_bar"] = false;
    global_patch["show_profile_name_in_menu_bar"] = true;
    global_patch["show_additional_menu_items"] = true;
    global_patch["enable_notification_window"] = false;
    global_patch["notification_window_position"] = "top_left";
    global_patch["notification_window_respect_screen_visible_frame"] = false;
    global_patch["notification_window_show_icon"] = false;
    global_patch["notification_window_font_size"] = 18;
    global_patch["notification_window_colors"]["light"]["background_color"] = "#11223344";
    global_patch["notification_window_colors"]["light"]["text_color"] = "#55667788";
    global_patch["notification_window_colors"]["dark"]["background_color"] = "#99AABBCC";
    global_patch["notification_window_colors"]["dark"]["text_color"] = "#DDEEFF00";
    global_patch["unsafe_ui"] = true;
    global_patch["filter_useless_events_from_specific_devices"] = false;
    global_patch["reorder_same_timestamp_input_events_to_prioritize_modifiers"] = false;
    global_patch["enable_cgeventtap_fallback"] = true;
    global_patch["delay_milliseconds_before_sleep_shortcut"] = 123;

    auto& machine_specific_patch = patch["machine_specific"];
    machine_specific_patch["enable_multitouch_extension"] = true;
    machine_specific_patch["external_editor_path"] = "/Applications/Editor.app";

    // Apply the patch and verify that it is recognized as a change.
    expect(settings_configuration_updater::apply_patch(patch, core_configuration));

    // Verify the updated global settings and normalized color values.
    const auto& global = core_configuration.get_global_configuration();
    expect(!global.get_check_for_updates());
    expect(!global.get_show_in_menu_bar());
    expect(global.get_show_profile_name_in_menu_bar());
    expect(global.get_show_additional_menu_items());
    expect(!global.get_enable_notification_window());
    expect(global.get_notification_window_position() == "top_left");
    expect(!global.get_notification_window_respect_screen_visible_frame());
    expect(!global.get_notification_window_show_icon());
    expect(global.get_notification_window_font_size() == 18_i);
    expect(global.get_notification_window_colors().get_light().get_background_color() == "#11223344");
    expect(global.get_notification_window_colors().get_light().get_text_color() == "#55667788");
    expect(global.get_notification_window_colors().get_dark().get_background_color() == "#99aabbcc");
    expect(global.get_notification_window_colors().get_dark().get_text_color() == "#ddeeff00");
    expect(global.get_unsafe_ui());
    expect(!global.get_filter_useless_events_from_specific_devices());
    expect(!global.get_reorder_same_timestamp_input_events_to_prioritize_modifiers());
    expect(global.get_enable_cgeventtap_fallback());
    expect(global.get_delay_milliseconds_before_sleep_shortcut() == 123_i);

    // Verify the updated machine-specific settings.
    const auto& machine_specific = core_configuration.get_machine_specific().get_entry();
    expect(machine_specific.get_enable_multitouch_extension());
    expect(machine_specific.get_external_editor_path() == "/Applications/Editor.app");
  };

  "settings_configuration_updater.apply_patch updates selected-profile settings"_test = [] {
    // Set up a default configuration.
    krbn::core_configuration::core_configuration core_configuration;

    // Build a patch that covers profile defaults, profile parameters, complex
    // modification parameters, and virtual HID keyboard settings.
    auto patch = nlohmann::json::object();
    auto& profile_patch = patch["selected_profile"];
    profile_patch["ignore_pointing_device_events_by_default"] = false;
    profile_patch["parameters"]["delay_milliseconds_before_open_device"] = 321;
    auto& complex_parameters_patch = profile_patch["complex_modifications"]["parameters"];
    complex_parameters_patch["basic_simultaneous_threshold_milliseconds"] = 55;
    complex_parameters_patch["basic_to_if_alone_timeout_milliseconds"] = 1100;
    complex_parameters_patch["basic_to_if_held_down_threshold_milliseconds"] = 600;
    complex_parameters_patch["basic_to_delayed_action_delay_milliseconds"] = 700;
    complex_parameters_patch["mouse_motion_to_scroll_speed"] = 120;
    auto& virtual_hid_keyboard_patch = profile_patch["virtual_hid_keyboard"];
    virtual_hid_keyboard_patch["keyboard_type_v2"] = "jis";
    virtual_hid_keyboard_patch["mouse_key_xy_scale"] = 150;
    virtual_hid_keyboard_patch["indicate_sticky_modifier_keys_state"] = false;

    // Apply the patch and verify that it is recognized as a change.
    expect(settings_configuration_updater::apply_patch(patch, core_configuration));

    // Verify the updated profile default and profile parameters.
    const auto& profile = core_configuration.get_selected_profile();
    expect(!profile.get_ignore_pointing_device_events_by_default());
    expect(profile.get_parameters()->get_delay_milliseconds_before_open_device() ==
           std::chrono::milliseconds(321));

    // Verify the updated complex modification parameters.
    const auto& complex_parameters = profile.get_complex_modifications()->get_parameters();
    expect(complex_parameters->get_basic_simultaneous_threshold_milliseconds() == 55_i);
    expect(complex_parameters->get_basic_to_if_alone_timeout_milliseconds() == 1100_i);
    expect(complex_parameters->get_basic_to_if_held_down_threshold_milliseconds() == 600_i);
    expect(complex_parameters->get_basic_to_delayed_action_delay_milliseconds() == 700_i);
    expect(complex_parameters->get_mouse_motion_to_scroll_speed() == 120_i);

    // Verify the updated virtual HID keyboard settings.
    const auto& virtual_hid_keyboard = profile.get_virtual_hid_keyboard();
    expect(virtual_hid_keyboard->get_keyboard_type_v2() == "jis");
    expect(virtual_hid_keyboard->get_mouse_key_xy_scale() == 150_i);
    expect(!virtual_hid_keyboard->get_indicate_sticky_modifier_keys_state());
  };

  "settings_configuration_updater.apply_patch updates device settings"_test = [] {
    // Set up a default configuration and a pointing-device identifier.
    krbn::core_configuration::core_configuration core_configuration;
    const auto identifiers = make_pointing_device_identifiers();

    // Build a patch that covers every device setting handled by apply_patch.
    auto patch = nlohmann::json::object();
    auto& device_patch = patch["selected_profile"]["devices"][identifiers.to_normalized_json().dump()];
    device_patch["ignore"] = true;
    device_patch["manipulate_caps_lock_led"] = true;
    device_patch["swap_grave_accent_and_non_us_backslash"] = true;
    device_patch["ignore_vendor_events"] = true;
    device_patch["treat_as_built_in_keyboard"] = false;
    device_patch["disable_built_in_keyboard_if_exists"] = true;
    device_patch["pointing_motion_xy_multiplier"] = 1.5;
    device_patch["pointing_motion_wheels_multiplier"] = 2.5;
    device_patch["mouse_flip_x"] = true;
    device_patch["mouse_flip_y"] = true;
    device_patch["mouse_flip_vertical_wheel"] = true;
    device_patch["mouse_flip_horizontal_wheel"] = true;
    device_patch["mouse_discard_x"] = true;
    device_patch["mouse_discard_y"] = true;
    device_patch["mouse_discard_vertical_wheel"] = true;
    device_patch["mouse_discard_horizontal_wheel"] = true;
    device_patch["mouse_swap_xy"] = true;
    device_patch["mouse_swap_wheels"] = true;
    device_patch["game_pad_swap_sticks"] = true;
    device_patch["game_pad_xy_stick_deadzone"] = 0.1;
    device_patch["game_pad_xy_stick_delta_magnitude_detection_threshold"] = 0.2;
    device_patch["game_pad_xy_stick_continued_movement_absolute_magnitude_threshold"] = 0.3;
    device_patch["game_pad_xy_stick_continued_movement_interval_milliseconds"] = 10;
    device_patch["game_pad_wheels_stick_deadzone"] = 0.4;
    device_patch["game_pad_wheels_stick_delta_magnitude_detection_threshold"] = 0.5;
    device_patch["game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold"] = 0.6;
    device_patch["game_pad_wheels_stick_continued_movement_interval_milliseconds"] = 20;

    // Apply the patch and verify that it is recognized as a change.
    expect(settings_configuration_updater::apply_patch(patch, core_configuration));

    // Verify every updated device setting.
    const auto device = core_configuration.get_selected_profile().get_device(identifiers);
    expect(device->get_ignore());
    expect(device->get_ignore_configured());
    expect(device->get_manipulate_caps_lock_led());
    expect(device->get_swap_grave_accent_and_non_us_backslash());
    expect(device->get_ignore_vendor_events());
    expect(!device->get_treat_as_built_in_keyboard());
    expect(device->get_disable_built_in_keyboard_if_exists());
    expect(device->get_pointing_motion_xy_multiplier() == 1.5_d);
    expect(device->get_pointing_motion_wheels_multiplier() == 2.5_d);
    expect(device->get_mouse_flip_x());
    expect(device->get_mouse_flip_y());
    expect(device->get_mouse_flip_vertical_wheel());
    expect(device->get_mouse_flip_horizontal_wheel());
    expect(device->get_mouse_discard_x());
    expect(device->get_mouse_discard_y());
    expect(device->get_mouse_discard_vertical_wheel());
    expect(device->get_mouse_discard_horizontal_wheel());
    expect(device->get_mouse_swap_xy());
    expect(device->get_mouse_swap_wheels());
    expect(device->get_game_pad_swap_sticks());
    expect(device->get_game_pad_xy_stick_deadzone() == 0.1_d);
    expect(device->get_game_pad_xy_stick_delta_magnitude_detection_threshold() == 0.2_d);
    expect(device->get_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold() == 0.3_d);
    expect(device->get_game_pad_xy_stick_continued_movement_interval_milliseconds() == 10_i);
    expect(device->get_game_pad_wheels_stick_deadzone() == 0.4_d);
    expect(device->get_game_pad_wheels_stick_delta_magnitude_detection_threshold() == 0.5_d);
    expect(device->get_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold() == 0.6_d);
    expect(device->get_game_pad_wheels_stick_continued_movement_interval_milliseconds() == 20_i);
  };

  // A patch value represents an explicit UI change. The setter must run even
  // when the patched value equals the inherited raw value, so the per-device
  // setting is recorded as explicitly configured.
  "settings_configuration_updater.apply_patch records an explicit device setting"_test = [] {
    // Set up an unconfigured pointing device whose inherited ignore value is
    // already false.
    krbn::core_configuration::core_configuration core_configuration;
    auto& profile = core_configuration.get_selected_profile();
    profile.set_ignore_pointing_device_events_by_default(false);

    const auto identifiers = make_pointing_device_identifiers();
    auto device = profile.get_device(identifiers);

    // Verify the precondition: the raw value is false but is not explicit.
    expect(!device->get_ignore());
    expect(!device->get_ignore_configured());

    // Build a patch that explicitly sets the same false value.
    auto patch = nlohmann::json::object();
    patch["selected_profile"]["devices"][identifiers.to_normalized_json().dump()]["ignore"] = false;

    // Apply the patch and verify that the value is now explicitly configured
    // and serialized even though the raw boolean value did not change.
    expect(settings_configuration_updater::apply_patch(patch, core_configuration));
    expect(!device->get_ignore());
    expect(device->get_ignore_configured());
    expect(device->to_json()["ignore"] == false);
  };

  return 0;
}
