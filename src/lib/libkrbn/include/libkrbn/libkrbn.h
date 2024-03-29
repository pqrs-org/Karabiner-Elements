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
} libkrbn_device_identifiers;

void libkrbn_set_logging_level_off(void);
void libkrbn_set_logging_level_info(void);

void libkrbn_initialize(void);
void libkrbn_terminate(void);

void libkrbn_get_distributed_notification_observed_object(char* buffer,
                                                          size_t length);
void libkrbn_get_distributed_notification_console_user_server_is_disabled(char* buffer,
                                                                          size_t length);
void libkrbn_get_user_configuration_directory(char* buffer,
                                              size_t length);
void libkrbn_get_user_complex_modifications_assets_directory(char* buffer,
                                                             size_t length);
void libkrbn_get_system_app_icon_configuration_file_path(char* buffer,
                                                         size_t length);

bool libkrbn_user_pid_directory_writable(void);

bool libkrbn_lock_single_application_with_user_pid_file(const char* pid_file_name);
void libkrbn_unlock_single_application(void);

void libkrbn_launchctl_manage_console_user_server(bool load);
void libkrbn_launchctl_manage_notification_window(bool load);
void libkrbn_launchctl_manage_session_monitor(void);
void libkrbn_launchctl_restart_console_user_server(void);
void libkrbn_launchctl_bootout_console_user_server(void);

void libkrbn_launch_event_viewer(void);
void libkrbn_launch_menu(void);
void libkrbn_launch_settings(void);
void libkrbn_launch_multitouch_extension(void);
void libkrbn_launch_uninstaller(void);

bool libkrbn_driver_running(void);
bool libkrbn_virtual_hid_keyboard_exists(void);
bool libkrbn_virtual_hid_pointing_exists(void);
bool libkrbn_system_core_configuration_file_path_exists(void);

// types

bool libkrbn_is_momentary_switch_event_target(int32_t usage_page, int32_t usage);
bool libkrbn_is_modifier_flag(int32_t usage_page, int32_t usage);
void libkrbn_get_momentary_switch_event_json_string(char* buffer, size_t length, int32_t usage_page, int32_t usage);
void libkrbn_get_momentary_switch_event_usage_name(char* buffer, size_t length, int32_t usage_page, int32_t usage);
void libkrbn_get_modifier_flag_name(char* buffer, size_t length, int32_t usage_page, int32_t usage);
void libkrbn_get_simple_modification_json_string(char* buffer, size_t length, int32_t usage_page, int32_t usage);

//
// libkrbn_core_configuration
//

bool libkrbn_core_configuration_save(char* error_message_buffer,
                                     size_t error_message_buffer_length);

// global_configuration

bool libkrbn_core_configuration_get_global_configuration_check_for_updates_on_startup(void);
void libkrbn_core_configuration_set_global_configuration_check_for_updates_on_startup(bool value);
bool libkrbn_core_configuration_get_global_configuration_show_in_menu_bar(void);
void libkrbn_core_configuration_set_global_configuration_show_in_menu_bar(bool value);
bool libkrbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar(void);
void libkrbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(bool value);
bool libkrbn_core_configuration_get_global_configuration_ask_for_confirmation_before_quitting(void);
void libkrbn_core_configuration_set_global_configuration_ask_for_confirmation_before_quitting(bool value);
bool libkrbn_core_configuration_get_global_configuration_unsafe_ui(void);
void libkrbn_core_configuration_set_global_configuration_unsafe_ui(bool value);

// profiles

size_t libkrbn_core_configuration_get_profiles_size(void);
bool libkrbn_core_configuration_get_profile_name(size_t index,
                                                 char* buffer,
                                                 size_t length);
void libkrbn_core_configuration_set_profile_name(size_t index, const char* value);
bool libkrbn_core_configuration_get_profile_selected(size_t index);
void libkrbn_core_configuration_select_profile(size_t index);
bool libkrbn_core_configuration_get_selected_profile_name(char* buffer,
                                                          size_t length);
void libkrbn_core_configuration_push_back_profile(void);
void libkrbn_core_configuration_duplicate_profile(size_t source_index);
void libkrbn_core_configuration_move_profile(size_t source_index, size_t destination_index);
void libkrbn_core_configuration_erase_profile(size_t index);

// profile::parameters

int libkrbn_core_configuration_get_selected_profile_parameters_delay_milliseconds_before_open_device(void);
void libkrbn_core_configuration_set_selected_profile_parameters_delay_milliseconds_before_open_device(int value);

// profile::simple_modifications

size_t libkrbn_core_configuration_get_selected_profile_simple_modifications_size(const libkrbn_device_identifiers* device_identifiers);
bool libkrbn_core_configuration_get_selected_profile_simple_modification_from_json_string(size_t index,
                                                                                          const libkrbn_device_identifiers* device_identifiers,
                                                                                          char* buffer,
                                                                                          size_t length);
bool libkrbn_core_configuration_get_selected_profile_simple_modification_to_json_string(size_t index,
                                                                                        const libkrbn_device_identifiers* device_identifiers,
                                                                                        char* buffer,
                                                                                        size_t length);
void libkrbn_core_configuration_replace_selected_profile_simple_modification(size_t index,
                                                                             const char* from_json_string,
                                                                             const char* to_json_string,
                                                                             const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_push_back_selected_profile_simple_modification(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_erase_selected_profile_simple_modification(size_t index,
                                                                           const libkrbn_device_identifiers* device_identifiers);

// profile::fn_function_keys

size_t libkrbn_core_configuration_get_selected_profile_fn_function_keys_size(const libkrbn_device_identifiers* device_identifiers);
bool libkrbn_core_configuration_get_selected_profile_fn_function_key_from_json_string(size_t index,
                                                                                      const libkrbn_device_identifiers* device_identifiers,
                                                                                      char* buffer,
                                                                                      size_t length);
bool libkrbn_core_configuration_get_selected_profile_fn_function_key_to_json_string(size_t index,
                                                                                    const libkrbn_device_identifiers* device_identifiers,
                                                                                    char* buffer,
                                                                                    size_t length);
void libkrbn_core_configuration_replace_selected_profile_fn_function_key(const char* from_json_string,
                                                                         const char* to_json_string,
                                                                         const libkrbn_device_identifiers* device_identifiers);

// profile:complex_modifications

size_t libkrbn_core_configuration_get_selected_profile_complex_modifications_rules_size(void);
bool libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_description(size_t index,
                                                                                            char* buffer,
                                                                                            size_t length);
bool libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_json_string(size_t index,
                                                                                            char* buffer,
                                                                                            size_t length);
void libkrbn_core_configuration_replace_selected_profile_complex_modifications_rule(size_t index,
                                                                                    const char* json_string,
                                                                                    char* error_message_buffer,
                                                                                    size_t error_message_buffer_length);
void libkrbn_core_configuration_push_front_selected_profile_complex_modifications_rule(const char* json_string,
                                                                                       char* error_message_buffer,
                                                                                       size_t error_message_buffer_length);
void libkrbn_core_configuration_erase_selected_profile_complex_modifications_rule(size_t index);
void libkrbn_core_configuration_move_selected_profile_complex_modifications_rule(size_t source_index, size_t destination_index);
int libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(const char* name);
void libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter(const char* name,
                                                                                     int value);
void libkrbn_core_configuration_get_new_complex_modifications_rule_json_string(char* buffer,
                                                                               size_t length);

// profile::virtual_hid_device

uint8_t libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_country_code(void);
void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_country_code(uint8_t value);

int libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(void);
void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(int value);

bool libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(void);
void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(bool value);

// profile::devices

bool libkrbn_core_configuration_get_selected_profile_device_ignore(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_ignore(const libkrbn_device_identifiers* device_identifiers,
                                                                   bool value);
bool libkrbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_manipulate_caps_lock_led(const libkrbn_device_identifiers* device_identifiers,
                                                                                     bool value);
bool libkrbn_core_configuration_get_selected_profile_device_treat_as_built_in_keyboard(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_treat_as_built_in_keyboard(const libkrbn_device_identifiers* device_identifiers,
                                                                                       bool value);
bool libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(const libkrbn_device_identifiers* device_identifiers,
                                                                                                bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_flip_x(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_flip_x(const libkrbn_device_identifiers* device_identifiers,
                                                                         bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_flip_y(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_flip_y(const libkrbn_device_identifiers* device_identifiers,
                                                                         bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_flip_vertical_wheel(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_flip_vertical_wheel(const libkrbn_device_identifiers* device_identifiers,
                                                                                      bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_flip_horizontal_wheel(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_flip_horizontal_wheel(const libkrbn_device_identifiers* device_identifiers,
                                                                                        bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_swap_xy(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_swap_xy(const libkrbn_device_identifiers* device_identifiers,
                                                                          bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_swap_wheels(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_swap_wheels(const libkrbn_device_identifiers* device_identifiers,
                                                                              bool value);
bool libkrbn_core_configuration_get_selected_profile_device_game_pad_swap_sticks(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_swap_sticks(const libkrbn_device_identifiers* device_identifiers,
                                                                                 bool value);

// game_pad_xy_stick_continued_movement_absolute_magnitude_threshold

bool libkrbn_core_configuration_has_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(const libkrbn_device_identifiers* device_identifiers);
double libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(const libkrbn_device_identifiers* device_identifiers,
                                                                                                                              double value);
void libkrbn_core_configuration_unset_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(const libkrbn_device_identifiers* device_identifiers);

// game_pad_xy_stick_continued_movement_interval_milliseconds

bool libkrbn_core_configuration_has_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(const libkrbn_device_identifiers* device_identifiers);
int libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(const libkrbn_device_identifiers* device_identifiers,
                                                                                                                       int value);
void libkrbn_core_configuration_unset_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(const libkrbn_device_identifiers* device_identifiers);

int libkrbn_core_configuration_game_pad_xy_stick_continued_movement_interval_milliseconds_default_value(void);

// game_pad_xy_stick_flicking_input_window_milliseconds

bool libkrbn_core_configuration_has_selected_profile_device_game_pad_xy_stick_flicking_input_window_milliseconds(const libkrbn_device_identifiers* device_identifiers);
int libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_flicking_input_window_milliseconds(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_flicking_input_window_milliseconds(const libkrbn_device_identifiers* device_identifiers,
                                                                                                                 int value);
void libkrbn_core_configuration_unset_selected_profile_device_game_pad_xy_stick_flicking_input_window_milliseconds(const libkrbn_device_identifiers* device_identifiers);

int libkrbn_core_configuration_game_pad_xy_stick_flicking_input_window_milliseconds_default_value(void);

// game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold

bool libkrbn_core_configuration_has_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(const libkrbn_device_identifiers* device_identifiers);
double libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(const libkrbn_device_identifiers* device_identifiers,
                                                                                                                                  double value);
void libkrbn_core_configuration_unset_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(const libkrbn_device_identifiers* device_identifiers);

// game_pad_wheels_stick_continued_movement_interval_milliseconds

bool libkrbn_core_configuration_has_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(const libkrbn_device_identifiers* device_identifiers);
int libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(const libkrbn_device_identifiers* device_identifiers,
                                                                                                                           int value);
void libkrbn_core_configuration_unset_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(const libkrbn_device_identifiers* device_identifiers);

int libkrbn_core_configuration_game_pad_wheels_stick_continued_movement_interval_milliseconds_default_value(void);

// game_pad_wheels_stick_flicking_input_window_milliseconds

bool libkrbn_core_configuration_has_selected_profile_device_game_pad_wheels_stick_flicking_input_window_milliseconds(const libkrbn_device_identifiers* device_identifiers);
int libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_flicking_input_window_milliseconds(const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_flicking_input_window_milliseconds(const libkrbn_device_identifiers* device_identifiers,
                                                                                                                     int value);
void libkrbn_core_configuration_unset_selected_profile_device_game_pad_wheels_stick_flicking_input_window_milliseconds(const libkrbn_device_identifiers* device_identifiers);

int libkrbn_core_configuration_game_pad_wheels_stick_flicking_input_window_milliseconds_default_value(void);

// game_pad_stick_x_formula

bool libkrbn_core_configuration_has_selected_profile_device_game_pad_stick_x_formula(const libkrbn_device_identifiers* device_identifiers);
bool libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_x_formula(const libkrbn_device_identifiers* device_identifiers,
                                                                                     char* buffer,
                                                                                     size_t length);
bool libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_x_formula(const libkrbn_device_identifiers* device_identifiers,
                                                                                     const char* value);
void libkrbn_core_configuration_unset_selected_profile_device_game_pad_stick_x_formula(const libkrbn_device_identifiers* device_identifiers);

// game_pad_stick_y_formula

bool libkrbn_core_configuration_has_selected_profile_device_game_pad_stick_y_formula(const libkrbn_device_identifiers* device_identifiers);
bool libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_y_formula(const libkrbn_device_identifiers* device_identifiers,
                                                                                     char* buffer,
                                                                                     size_t length);
bool libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_y_formula(const libkrbn_device_identifiers* device_identifiers,
                                                                                     const char* value);
void libkrbn_core_configuration_unset_selected_profile_device_game_pad_stick_y_formula(const libkrbn_device_identifiers* device_identifiers);

// game_pad_stick_vertical_wheel_formula

bool libkrbn_core_configuration_has_selected_profile_device_game_pad_stick_vertical_wheel_formula(const libkrbn_device_identifiers* device_identifiers);
bool libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_vertical_wheel_formula(const libkrbn_device_identifiers* device_identifiers,
                                                                                                  char* buffer,
                                                                                                  size_t length);
bool libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_vertical_wheel_formula(const libkrbn_device_identifiers* device_identifiers,
                                                                                                  const char* value);
void libkrbn_core_configuration_unset_selected_profile_device_game_pad_stick_vertical_wheel_formula(const libkrbn_device_identifiers* device_identifiers);

// game_pad_stick_horizontal_wheel_formula

bool libkrbn_core_configuration_has_selected_profile_device_game_pad_stick_horizontal_wheel_formula(const libkrbn_device_identifiers* device_identifiers);
bool libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_horizontal_wheel_formula(const libkrbn_device_identifiers* device_identifiers,
                                                                                                    char* buffer,
                                                                                                    size_t length);
bool libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_horizontal_wheel_formula(const libkrbn_device_identifiers* device_identifiers,
                                                                                                    const char* value);
void libkrbn_core_configuration_unset_selected_profile_device_game_pad_stick_horizontal_wheel_formula(const libkrbn_device_identifiers* device_identifiers);

// game_pad_*

bool libkrbn_core_configuration_game_pad_validate_stick_formula(const char* formula);

//
// libkrbn_complex_modifications_assets_manager
//

void libkrbn_enable_complex_modifications_assets_manager(void);
void libkrbn_disable_complex_modifications_assets_manager(void);

void libkrbn_complex_modifications_assets_manager_reload(void);

size_t libkrbn_complex_modifications_assets_manager_get_files_size(void);
bool libkrbn_complex_modifications_assets_manager_get_file_title(size_t index,
                                                                 char* buffer,
                                                                 size_t length);
time_t libkrbn_complex_modifications_assets_manager_get_file_last_write_time(size_t index);

size_t libkrbn_complex_modifications_assets_manager_get_rules_size(size_t file_index);
bool libkrbn_complex_modifications_assets_manager_get_rule_description(size_t file_index,
                                                                       size_t index,
                                                                       char* buffer,
                                                                       size_t length);

void libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(size_t file_index,
                                                                                                  size_t index);
bool libkrbn_complex_modifications_assets_manager_user_file(size_t index);
void libkrbn_complex_modifications_assets_manager_erase_file(size_t index);

//
// libkrbn_configuration_monitor
//

typedef void (*libkrbn_core_configuration_updated)(void);
void libkrbn_enable_configuration_monitor(void);
void libkrbn_disable_configuration_monitor(void);

void libkrbn_register_core_configuration_updated_callback(libkrbn_core_configuration_updated callback);
void libkrbn_unregister_core_configuration_updated_callback(libkrbn_core_configuration_updated callback);

//
// libkrbn_system_preferences_monitor
//

struct libkrbn_system_preferences_properties {
  bool use_fkeys_as_standard_function_keys;
  int32_t keyboard_types[8];
};

typedef void (*libkrbn_system_preferences_monitor_callback)(const struct libkrbn_system_preferences_properties* properties,
                                                            void* refcon);
void libkrbn_enable_system_preferences_monitor(libkrbn_system_preferences_monitor_callback callback,
                                               void* refcon);
void libkrbn_disable_system_preferences_monitor(void);
size_t libkrbn_system_preferences_properties_get_keyboard_types_size(void);

//
// libkrbn_connected_devices
//

size_t libkrbn_connected_devices_get_size(void);
bool libkrbn_connected_devices_get_descriptions_manufacturer(size_t index,
                                                             char* buffer,
                                                             size_t length);
bool libkrbn_connected_devices_get_descriptions_product(size_t index,
                                                        char* buffer,
                                                        size_t length);
bool libkrbn_connected_devices_get_descriptions_transport(size_t index,
                                                          char* buffer,
                                                          size_t length);
bool libkrbn_connected_devices_get_device_identifiers(size_t index,
                                                      libkrbn_device_identifiers* device_identifiers);
uint64_t libkrbn_connected_devices_get_vendor_id(size_t index);
uint64_t libkrbn_connected_devices_get_product_id(size_t index);
bool libkrbn_connected_devices_get_device_address(size_t index,
                                                  char* buffer,
                                                  size_t length);
bool libkrbn_connected_devices_get_is_keyboard(size_t index);
bool libkrbn_connected_devices_get_is_pointing_device(size_t index);
bool libkrbn_connected_devices_get_is_game_pad(size_t index);
bool libkrbn_connected_devices_get_is_built_in_keyboard(size_t index);
bool libkrbn_connected_devices_get_is_built_in_trackpad(size_t index);
bool libkrbn_connected_devices_get_is_built_in_touch_bar(size_t index);
bool libkrbn_connected_devices_is_apple(size_t index);

typedef void (*libkrbn_connected_devices_updated)(void);
void libkrbn_enable_connected_devices_monitor(void);
void libkrbn_disable_connected_devices_monitor(void);

void libkrbn_register_connected_devices_updated_callback(libkrbn_connected_devices_updated callback);
void libkrbn_unregister_connected_devices_updated_callback(libkrbn_connected_devices_updated callback);

//
// libkrbn_version_monitor
//

typedef void (*libkrbn_version_monitor_callback)(void* refcon);
void libkrbn_enable_version_monitor(libkrbn_version_monitor_callback callback,
                                    void* refcon);
void libkrbn_disable_version_monitor(void);

//
// libkrbn_file_monitor
//

typedef void (*libkrbn_file_monitor_callback)(const char* file_path,
                                              void* refcon);

// libkrbn_observer_state_json_file_monitor

void libkrbn_enable_observer_state_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                     void* refcon);
void libkrbn_disable_observer_state_json_file_monitor(void);

// libkrbn_grabber_state_json_file_monitor

void libkrbn_enable_grabber_state_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                    void* refcon);
void libkrbn_disable_grabber_state_json_file_monitor(void);

// libkrbn_device_details_json_file_monitor

void libkrbn_enable_device_details_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                     void* refcon);
void libkrbn_disable_device_details_json_file_monitor(void);

// libkrbn_manipulator_environment_json_file_monitor

void libkrbn_enable_manipulator_environment_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                              void* refcon);
void libkrbn_disable_manipulator_environment_json_file_monitor(void);

// libkrbn_notification_message_json_file_monitor

void libkrbn_enable_notification_message_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                           void* refcon);
void libkrbn_disable_notification_message_json_file_monitor(void);

bool libkrbn_get_notification_message_body(char* buffer,
                                           size_t length);

//
// libkrbn_frontmost_application_monitor
//

typedef void (*libkrbn_frontmost_application_monitor_callback)(const char* bundle_identifier,
                                                               const char* file_path,
                                                               void* refcon);
void libkrbn_enable_frontmost_application_monitor(libkrbn_frontmost_application_monitor_callback callback,
                                                  void* refcon);
void libkrbn_disable_frontmost_application_monitor(void);

//
// libkrbn_log_monitor
//

typedef void (*libkrbn_log_messages_updated)(void);
void libkrbn_enable_log_monitor(void);
void libkrbn_disable_log_monitor(void);

void libkrbn_register_log_messages_updated_callback(libkrbn_log_messages_updated callback);
void libkrbn_unregister_log_messages_updated_callback(libkrbn_log_messages_updated callback);

size_t libkrbn_log_lines_get_size(void);
bool libkrbn_log_lines_get_line(size_t index,
                                char* buffer,
                                size_t length);
bool libkrbn_log_lines_is_warn_line(const char* line);
bool libkrbn_log_lines_is_error_line(const char* line);
uint64_t libkrbn_log_lines_get_date_number(const char* line);

//
// libkrbn_hid_value_monitor
//

typedef enum {
  libkrbn_hid_value_event_type_key_down,
  libkrbn_hid_value_event_type_key_up,
  libkrbn_hid_value_event_type_single,
} libkrbn_hid_value_event_type;

typedef void (*libkrbn_hid_value_monitor_callback)(uint64_t device_id,
                                                   int32_t usage_page,
                                                   int32_t usage,
                                                   int64_t logical_max,
                                                   int64_t logical_min,
                                                   int64_t integet_value,
                                                   void* refcon);
void libkrbn_enable_hid_value_monitor(libkrbn_hid_value_monitor_callback callback,
                                      void* refcon);
void libkrbn_disable_hid_value_monitor(void);
bool libkrbn_hid_value_monitor_observed(void);

//
// libkrbn_grabber_client
//

typedef void (*libkrbn_grabber_client_connected_callback)(void);
typedef void (*libkrbn_grabber_client_connect_failed_callback)(void);
typedef void (*libkrbn_grabber_client_closed_callback)(void);

void libkrbn_enable_grabber_client(libkrbn_grabber_client_connected_callback connected_callback,
                                   libkrbn_grabber_client_connect_failed_callback connect_failed_callback,
                                   libkrbn_grabber_client_closed_callback closed_callback);
void libkrbn_disable_grabber_client(void);
void libkrbn_grabber_client_async_set_app_icon(int number);
void libkrbn_grabber_client_async_set_keyboard_type(uint64_t country_code, uint64_t keyboard_type);
void libkrbn_grabber_client_async_set_variable(const char* name, int value);
void libkrbn_grabber_client_sync_set_variable(const char* name, int value);

#ifdef __cplusplus
}
#endif
