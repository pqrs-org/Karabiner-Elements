#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint64_t vendor_id;
  uint64_t product_id;
  char device_address[1024];
  bool is_keyboard;
  bool is_pointing_device;
  bool is_game_pad;
  bool is_consumer;
  bool is_virtual_device;
} libkrbn_device_identifiers;

void libkrbn_set_logging_level_off(void);
void libkrbn_set_logging_level_info(void);

void libkrbn_initialize(void);
void libkrbn_terminate(void);

void libkrbn_load_custom_environment_variables(void);

typedef void (*libkrbn_callback_t)(void);
void libkrbn_enqueue_callback(libkrbn_callback_t _Nonnull callback);

void libkrbn_get_user_configuration_directory(char* _Nonnull buffer,
                                              size_t length);
void libkrbn_get_user_complex_modifications_assets_directory(char* _Nonnull buffer,
                                                             size_t length);

bool libkrbn_user_pid_directory_writable(void);

bool libkrbn_lock_single_application_with_user_pid_file(const char* _Nonnull pid_file_name);
void libkrbn_unlock_single_application(void);

void libkrbn_services_register_core_daemons(void);
void libkrbn_services_register_core_agents(void);
void libkrbn_services_bootout_old_agents(void);
void libkrbn_services_restart_console_user_server_agent(void);
void libkrbn_services_unregister_all_agents(void);
bool libkrbn_services_core_daemons_enabled(void);
bool libkrbn_services_core_agents_enabled(void);
bool libkrbn_services_daemon_running(const char* _Nonnull service_name);
bool libkrbn_services_agent_running(const char* _Nonnull service_name);
bool libkrbn_services_core_daemons_running(void);
bool libkrbn_services_core_agents_running(void);

void libkrbn_updater_check_for_updates_stable_only(void);
void libkrbn_updater_check_for_updates_with_beta_version(void);

void libkrbn_launch_event_viewer(void);
void libkrbn_launch_settings(void);
void libkrbn_killall_settings(void);
void libkrbn_launch_uninstaller(void);

bool libkrbn_driver_running(void);
bool libkrbn_virtual_hid_keyboard_exists(void);
bool libkrbn_virtual_hid_pointing_exists(void);
bool libkrbn_system_core_configuration_file_path_exists(void);

bool libkrbn_system_preferences_virtual_hid_keyboard_modifier_mappings_exists(void);

int librkbn_get_app_icon_number(void);

// types

bool libkrbn_is_momentary_switch_event_target(int32_t usage_page, int32_t usage);
bool libkrbn_is_modifier_flag(int32_t usage_page, int32_t usage);
void libkrbn_get_momentary_switch_event_json_string(char* _Nonnull buffer,
                                                    size_t length,
                                                    int32_t usage_page,
                                                    int32_t usage);
void libkrbn_get_momentary_switch_event_usage_name(char* _Nonnull buffer,
                                                   size_t length,
                                                   int32_t usage_page,
                                                   int32_t usage);
void libkrbn_get_modifier_flag_name(char* _Nonnull buffer,
                                    size_t length,
                                    int32_t usage_page,
                                    int32_t usage);
void libkrbn_get_simple_modification_json_string(char* _Nonnull buffer,
                                                 size_t length,
                                                 int32_t usage_page,
                                                 int32_t usage);

//
// libkrbn_core_configuration
//

bool libkrbn_core_configuration_save(char* _Nonnull error_message_buffer,
                                     size_t error_message_buffer_length);

// global_configuration

bool libkrbn_core_configuration_get_global_configuration_check_for_updates_on_startup(void);
void libkrbn_core_configuration_set_global_configuration_check_for_updates_on_startup(bool value);
bool libkrbn_core_configuration_get_global_configuration_show_in_menu_bar(void);
void libkrbn_core_configuration_set_global_configuration_show_in_menu_bar(bool value);
bool libkrbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar(void);
void libkrbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(bool value);
bool libkrbn_core_configuration_get_global_configuration_show_additional_menu_items(void);
void libkrbn_core_configuration_set_global_configuration_show_additional_menu_items(bool value);
bool libkrbn_core_configuration_get_global_configuration_enable_notification_window(void);
void libkrbn_core_configuration_set_global_configuration_enable_notification_window(bool value);
bool libkrbn_core_configuration_get_global_configuration_ask_for_confirmation_before_quitting(void);
void libkrbn_core_configuration_set_global_configuration_ask_for_confirmation_before_quitting(bool value);
bool libkrbn_core_configuration_get_global_configuration_unsafe_ui(void);
void libkrbn_core_configuration_set_global_configuration_unsafe_ui(bool value);
bool libkrbn_core_configuration_get_global_configuration_filter_useless_events_from_specific_devices(void);
void libkrbn_core_configuration_set_global_configuration_filter_useless_events_from_specific_devices(bool value);
bool libkrbn_core_configuration_get_global_configuration_reorder_same_timestamp_input_events_to_prioritize_modifiers(void);
void libkrbn_core_configuration_set_global_configuration_reorder_same_timestamp_input_events_to_prioritize_modifiers(bool value);

// machine_specific

bool libkrbn_core_configuration_get_machine_specific_enable_multitouch_extension(void);
void libkrbn_core_configuration_set_machine_specific_enable_multitouch_extension(bool value);

// profiles

size_t libkrbn_core_configuration_get_profiles_size(void);
bool libkrbn_core_configuration_get_profile_name(size_t index,
                                                 char* _Nonnull buffer,
                                                 size_t length);
void libkrbn_core_configuration_set_profile_name(size_t index,
                                                 const char* _Nonnull value);
bool libkrbn_core_configuration_get_profile_selected(size_t index);
void libkrbn_core_configuration_select_profile(size_t index);
bool libkrbn_core_configuration_get_selected_profile_name(char* _Nonnull buffer,
                                                          size_t length);
void libkrbn_core_configuration_push_back_profile(void);
void libkrbn_core_configuration_duplicate_profile(size_t source_index);
void libkrbn_core_configuration_move_profile(size_t source_index, size_t destination_index);
void libkrbn_core_configuration_erase_profile(size_t index);

// profile::parameters

int libkrbn_core_configuration_get_selected_profile_parameters_delay_milliseconds_before_open_device(void);
void libkrbn_core_configuration_set_selected_profile_parameters_delay_milliseconds_before_open_device(int value);

// profile::simple_modifications

size_t libkrbn_core_configuration_get_selected_profile_simple_modifications_size(const libkrbn_device_identifiers* _Nullable device_identifiers);
bool libkrbn_core_configuration_get_selected_profile_simple_modification_from_json_string(size_t index,
                                                                                          const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                          char* _Nonnull buffer,
                                                                                          size_t length);
bool libkrbn_core_configuration_get_selected_profile_simple_modification_to_json_string(size_t index,
                                                                                        const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                        char* _Nonnull buffer,
                                                                                        size_t length);
void libkrbn_core_configuration_replace_selected_profile_simple_modification(size_t index,
                                                                             const char* _Nonnull from_json_string,
                                                                             const char* _Nonnull to_json_string,
                                                                             const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_push_back_selected_profile_simple_modification(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_erase_selected_profile_simple_modification(size_t index,
                                                                           const libkrbn_device_identifiers* _Nullable device_identifiers);

// profile::fn_function_keys

size_t libkrbn_core_configuration_get_selected_profile_fn_function_keys_size(const libkrbn_device_identifiers* _Nullable device_identifiers);
bool libkrbn_core_configuration_get_selected_profile_fn_function_key_from_json_string(size_t index,
                                                                                      const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                      char* _Nonnull buffer,
                                                                                      size_t length);
bool libkrbn_core_configuration_get_selected_profile_fn_function_key_to_json_string(size_t index,
                                                                                    const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                    char* _Nonnull buffer,
                                                                                    size_t length);
void libkrbn_core_configuration_replace_selected_profile_fn_function_key(const char* _Nonnull from_json_string,
                                                                         const char* _Nonnull to_json_string,
                                                                         const libkrbn_device_identifiers* _Nullable device_identifiers);

// profile:complex_modifications

size_t libkrbn_core_configuration_get_selected_profile_complex_modifications_rules_size(void);
bool libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_description(size_t index,
                                                                                            char* _Nonnull buffer,
                                                                                            size_t length);
bool libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_enabled(size_t index);
void libkrbn_core_configuration_set_selected_profile_complex_modifications_rule_enabled(size_t index, bool value);
bool libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_json_string(size_t index,
                                                                                            char* _Nonnull buffer,
                                                                                            size_t length);
void libkrbn_core_configuration_replace_selected_profile_complex_modifications_rule(size_t index,
                                                                                    const char* _Nonnull json_string,
                                                                                    char* _Nonnull error_message_buffer,
                                                                                    size_t error_message_buffer_length);
void libkrbn_core_configuration_push_front_selected_profile_complex_modifications_rule(const char* _Nonnull json_string,
                                                                                       char* _Nonnull error_message_buffer,
                                                                                       size_t error_message_buffer_length);
void libkrbn_core_configuration_erase_selected_profile_complex_modifications_rule(size_t index);
void libkrbn_core_configuration_move_selected_profile_complex_modifications_rule(size_t source_index, size_t destination_index);

int libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_simultaneous_threshold_milliseconds(void);
void libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_simultaneous_threshold_milliseconds(int value);

int libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_to_if_alone_timeout_milliseconds(void);
void libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_if_alone_timeout_milliseconds(int value);

int libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_to_if_held_down_threshold_milliseconds(void);
void libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_if_held_down_threshold_milliseconds(int value);

int libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_to_delayed_action_delay_milliseconds(void);
void libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_delayed_action_delay_milliseconds(int value);

int libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter_mouse_motion_to_scroll_speed(void);
void libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter_mouse_motion_to_scroll_speed(int value);

void libkrbn_core_configuration_get_new_complex_modifications_rule_json_string(char* _Nonnull buffer,
                                                                               size_t length);

// profile::virtual_hid_device

void libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_keyboard_type_v2(char* _Nonnull buffer,
                                                                                           size_t length);
void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_keyboard_type_v2(const char* _Nonnull value);

int libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(void);
void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(int value);

bool libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(void);
void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(bool value);

// profile::devices

bool libkrbn_core_configuration_get_selected_profile_device_ignore(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_ignore(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                   bool value);
bool libkrbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_manipulate_caps_lock_led(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                     bool value);
bool libkrbn_core_configuration_get_selected_profile_device_ignore_vendor_events(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_ignore_vendor_events(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                 bool value);
bool libkrbn_core_configuration_get_selected_profile_device_treat_as_built_in_keyboard(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_treat_as_built_in_keyboard(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                       bool value);
bool libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                                bool value);
double libkrbn_core_configuration_get_selected_profile_device_pointing_motion_xy_multiplier(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_pointing_motion_xy_multiplier(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                          double value);
double libkrbn_core_configuration_pointing_motion_xy_multiplier_default_value(void);
double libkrbn_core_configuration_get_selected_profile_device_pointing_motion_wheels_multiplier(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_pointing_motion_wheels_multiplier(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                              double value);
double libkrbn_core_configuration_pointing_motion_wheels_multiplier_default_value(void);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_flip_x(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_flip_x(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                         bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_flip_y(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_flip_y(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                         bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_flip_vertical_wheel(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_flip_vertical_wheel(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                      bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_flip_horizontal_wheel(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_flip_horizontal_wheel(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                        bool value);

bool libkrbn_core_configuration_get_selected_profile_device_mouse_discard_x(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_discard_x(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                            bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_discard_y(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_discard_y(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                            bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_discard_vertical_wheel(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_discard_vertical_wheel(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                         bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_discard_horizontal_wheel(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_discard_horizontal_wheel(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                           bool value);

bool libkrbn_core_configuration_get_selected_profile_device_mouse_swap_xy(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_swap_xy(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                          bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_swap_wheels(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_swap_wheels(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                              bool value);
bool libkrbn_core_configuration_get_selected_profile_device_game_pad_swap_sticks(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_swap_sticks(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                 bool value);

size_t libkrbn_core_configuration_get_selected_profile_not_connected_configured_devices_count(const char* _Nonnull connected_devices_json);
void libkrbn_core_configuration_erase_selected_profile_not_connected_configured_devices(const char* _Nonnull connected_devices_json);

// game_pad_xy_stick_deadzone

double libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_deadzone(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_deadzone(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                       double value);
double libkrbn_core_configuration_game_pad_xy_stick_deadzone_default_value(void);

// game_pad_xy_stick_delta_magnitude_detection_threshold

double libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_delta_magnitude_detection_threshold(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_delta_magnitude_detection_threshold(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                                                  double value);
double libkrbn_core_configuration_game_pad_xy_stick_delta_magnitude_detection_threshold_default_value(void);

// game_pad_xy_stick_continued_movement_absolute_magnitude_threshold

double libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                                                              double value);
double libkrbn_core_configuration_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold_default_value(void);

// game_pad_xy_stick_continued_movement_interval_milliseconds

int libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                                                       int value);
int libkrbn_core_configuration_game_pad_xy_stick_continued_movement_interval_milliseconds_default_value(void);

// game_pad_wheels_stick_deadzone

double libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_deadzone(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_deadzone(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                           double value);
double libkrbn_core_configuration_game_pad_wheels_stick_deadzone_default_value(void);

// game_pad_wheels_stick_delta_magnitude_detection_threshold

double libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_delta_magnitude_detection_threshold(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_delta_magnitude_detection_threshold(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                                                      double value);
double libkrbn_core_configuration_game_pad_wheels_stick_delta_magnitude_detection_threshold_default_value(void);

// game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold

double libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                                                                  double value);
double libkrbn_core_configuration_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold_default_value(void);

// game_pad_wheels_stick_continued_movement_interval_milliseconds

int libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                                                           int value);
int libkrbn_core_configuration_game_pad_wheels_stick_continued_movement_interval_milliseconds_default_value(void);

// game_pad_stick_x_formula

bool libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_x_formula(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                     char* _Nonnull buffer,
                                                                                     size_t length);
bool libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_x_formula(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                     const char* _Nonnull value);
void libkrbn_core_configuration_reset_selected_profile_device_game_pad_stick_x_formula(const libkrbn_device_identifiers* _Nullable device_identifiers);

// game_pad_stick_y_formula

bool libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_y_formula(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                     char* _Nonnull buffer,
                                                                                     size_t length);
bool libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_y_formula(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                     const char* _Nonnull value);
void libkrbn_core_configuration_reset_selected_profile_device_game_pad_stick_y_formula(const libkrbn_device_identifiers* _Nullable device_identifiers);

// game_pad_stick_vertical_wheel_formula

bool libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_vertical_wheel_formula(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                                  char* _Nonnull buffer,
                                                                                                  size_t length);
bool libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_vertical_wheel_formula(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                                  const char* _Nonnull value);
void libkrbn_core_configuration_reset_selected_profile_device_game_pad_stick_vertical_wheel_formula(const libkrbn_device_identifiers* _Nullable device_identifiers);

// game_pad_stick_horizontal_wheel_formula

bool libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_horizontal_wheel_formula(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                                    char* _Nonnull buffer,
                                                                                                    size_t length);
bool libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_horizontal_wheel_formula(const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                                    const char* _Nonnull value);
void libkrbn_core_configuration_reset_selected_profile_device_game_pad_stick_horizontal_wheel_formula(const libkrbn_device_identifiers* _Nullable device_identifiers);

// game_pad_*

bool libkrbn_core_configuration_game_pad_validate_stick_formula(const char* _Nonnull formula);

//
// libkrbn_complex_modifications_assets_manager
//

void libkrbn_enable_complex_modifications_assets_manager(void);
void libkrbn_disable_complex_modifications_assets_manager(void);

void libkrbn_complex_modifications_assets_manager_reload(void);

size_t libkrbn_complex_modifications_assets_manager_get_files_size(void);
bool libkrbn_complex_modifications_assets_manager_get_file_title(size_t index,
                                                                 char* _Nonnull buffer,
                                                                 size_t length);
time_t libkrbn_complex_modifications_assets_manager_get_file_last_write_time(size_t index);

size_t libkrbn_complex_modifications_assets_manager_get_rules_size(size_t file_index);
bool libkrbn_complex_modifications_assets_manager_get_rule_description(size_t file_index,
                                                                       size_t index,
                                                                       char* _Nonnull buffer,
                                                                       size_t length);

void libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(size_t file_index,
                                                                                                  size_t index);
bool libkrbn_complex_modifications_assets_manager_user_file(size_t index);
void libkrbn_complex_modifications_assets_manager_erase_file(size_t index);

//
// libkrbn_configuration_monitor
//

void libkrbn_enable_configuration_monitor(void);
void libkrbn_disable_configuration_monitor(void);

typedef void (*libkrbn_core_configuration_updated_t)(void);
void libkrbn_register_core_configuration_updated_callback(libkrbn_core_configuration_updated_t _Nonnull callback);
void libkrbn_unregister_core_configuration_updated_callback(libkrbn_core_configuration_updated_t _Nonnull callback);

bool libkrbn_configuration_monitor_get_parse_error_message(char* _Nonnull buffer,
                                                           size_t length);

//
// libkrbn_version_monitor
//

void libkrbn_enable_version_monitor(void);
void libkrbn_disable_version_monitor(void);

typedef void (*libkrbn_version_updated_t)(void);
void libkrbn_register_version_updated_callback(libkrbn_version_updated_t _Nonnull callback);
void libkrbn_unregister_version_updated_callback(libkrbn_version_updated_t _Nonnull callback);

bool libkrbn_get_version(char* _Nonnull buffer,
                         size_t length);

//
// libkrbn_file_monitors
//

void libkrbn_enable_file_monitors(void);
void libkrbn_disable_file_monitors(void);

typedef void (*libkrbn_file_updated_t)(void);
void libkrbn_register_file_updated_callback(const char* _Nonnull file_path,
                                            libkrbn_file_updated_t _Nonnull callback);
void libkrbn_unregister_file_updated_callback(const char* _Nonnull file_path,
                                              libkrbn_file_updated_t _Nonnull callback);

void libkrbn_get_core_service_state_json_file_path(char* _Nonnull buffer, size_t length);
void libkrbn_get_notification_message_json_file_path(char* _Nonnull buffer, size_t length);

//
// libkrbn_frontmost_application_monitor
//

void libkrbn_enable_frontmost_application_monitor(void);
void libkrbn_disable_frontmost_application_monitor(void);

typedef void (*libkrbn_frontmost_application_changed_t)(void);
void libkrbn_register_frontmost_application_changed_callback(libkrbn_frontmost_application_changed_t _Nonnull callback);
void libkrbn_unregister_frontmost_application_changed_callback(libkrbn_frontmost_application_changed_t _Nonnull callback);

bool libkrbn_get_frontmost_application(char* _Nonnull bundle_identifier_buffer,
                                       size_t bundle_identifier_buffer_length,
                                       char* _Nonnull file_path_buffer,
                                       size_t file_path_buffer_length);

//
// libkrbn_log_monitor
//

void libkrbn_enable_log_monitor(void);
void libkrbn_disable_log_monitor(void);

typedef void (*libkrbn_log_messages_updated_t)(void);
void libkrbn_register_log_messages_updated_callback(libkrbn_log_messages_updated_t _Nonnull callback);
void libkrbn_unregister_log_messages_updated_callback(libkrbn_log_messages_updated_t _Nonnull callback);

size_t libkrbn_log_lines_get_size(void);
bool libkrbn_log_lines_get_line(size_t index,
                                char* _Nonnull buffer,
                                size_t length);
bool libkrbn_log_lines_is_warn_line(const char* _Nonnull line);
bool libkrbn_log_lines_is_error_line(const char* _Nonnull line);
uint64_t libkrbn_log_lines_get_date_number(const char* _Nonnull line);

//
// libkrbn_hid_value_monitor
//

void libkrbn_enable_hid_value_monitor(void);
void libkrbn_disable_hid_value_monitor(void);

typedef void (*libkrbn_hid_value_arrived_t)(uint64_t device_id,
                                            bool is_keyboard,
                                            bool is_pointing_device,
                                            bool is_game_pad,
                                            int32_t usage_page,
                                            int32_t usage,
                                            int64_t logical_max,
                                            int64_t logical_min,
                                            int64_t integet_value);
void libkrbn_register_hid_value_arrived_callback(libkrbn_hid_value_arrived_t _Nonnull callback);
void libkrbn_unregister_hid_value_arrived_callback(libkrbn_hid_value_arrived_t _Nonnull callback);

bool libkrbn_hid_value_monitor_observed(void);

//
// libkrbn_core_service_client
//

typedef enum {
  libkrbn_core_service_client_status_none,
  libkrbn_core_service_client_status_connected,
  libkrbn_core_service_client_status_connect_failed,
  libkrbn_core_service_client_status_closed,
} libkrbn_core_service_client_status;

void libkrbn_enable_core_service_client(const char* _Nullable client_socket_directory_name);
void libkrbn_disable_core_service_client(void);

void libkrbn_core_service_client_async_start(void);

typedef void (*libkrbn_core_service_client_status_changed_t)(void);
void libkrbn_register_core_service_client_status_changed_callback(libkrbn_core_service_client_status_changed_t _Nonnull callback);
void libkrbn_unregister_core_service_client_status_changed_callback(libkrbn_core_service_client_status_changed_t _Nonnull callback);

libkrbn_core_service_client_status libkrbn_core_service_client_get_status(void);

void libkrbn_core_service_client_async_temporarily_ignore_all_devices(bool value);

void libkrbn_core_service_client_async_get_manipulator_environment(void);
typedef void (*libkrbn_core_service_client_manipulator_environment_received_t)(const char* _Nonnull json_string);
void libkrbn_register_core_service_client_manipulator_environment_received_callback(libkrbn_core_service_client_manipulator_environment_received_t _Nonnull callback);
void libkrbn_unregister_core_service_client_manipulator_environment_received_callback(libkrbn_core_service_client_manipulator_environment_received_t _Nonnull callback);

void libkrbn_core_service_client_async_get_connected_devices(void);
typedef void (*libkrbn_core_service_client_connected_devices_received_t)(const char* _Nonnull json_string);
void libkrbn_register_core_service_client_connected_devices_received_callback(libkrbn_core_service_client_connected_devices_received_t _Nonnull callback);
void libkrbn_unregister_core_service_client_connected_devices_received_callback(libkrbn_core_service_client_connected_devices_received_t _Nonnull callback);

void libkrbn_core_service_client_async_get_notification_message(void);
typedef void (*libkrbn_core_service_client_notification_message_received_t)(const char* _Nonnull message);
void libkrbn_register_core_service_client_notification_message_received_callback(libkrbn_core_service_client_notification_message_received_t _Nonnull callback);
void libkrbn_unregister_core_service_client_notification_message_received_callback(libkrbn_core_service_client_notification_message_received_t _Nonnull callback);

void libkrbn_core_service_client_async_get_system_variables(void);
typedef void (*libkrbn_core_service_client_system_variables_received_t)(const char* _Nonnull json_string);
void libkrbn_register_core_service_client_system_variables_received_callback(libkrbn_core_service_client_system_variables_received_t _Nonnull callback);
void libkrbn_unregister_core_service_client_system_variables_received_callback(libkrbn_core_service_client_system_variables_received_t _Nonnull callback);

void libkrbn_core_service_client_async_connect_multitouch_extension(void);

void libkrbn_core_service_client_async_set_app_icon(int number);
void libkrbn_core_service_client_async_set_variable(const char* _Nonnull name,
                                                    int value);

#ifdef __cplusplus
}
#endif
