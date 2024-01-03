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
  const char* device_address;
  bool is_keyboard;
  bool is_pointing_device;
  bool is_game_pad;
} libkrbn_device_identifiers;

void libkrbn_set_logging_level_off(void);
void libkrbn_set_logging_level_info(void);

void libkrbn_initialize(void);
void libkrbn_terminate(void);

const char* libkrbn_get_distributed_notification_observed_object(void);
const char* libkrbn_get_distributed_notification_console_user_server_is_disabled(void);
const char* libkrbn_get_user_configuration_directory(void);
const char* libkrbn_get_user_complex_modifications_assets_directory(void);
const char* libkrbn_get_system_app_icon_configuration_file_path(void);

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

typedef void libkrbn_core_configuration;
void libkrbn_core_configuration_terminate(libkrbn_core_configuration** p);
void libkrbn_core_configuration_save(libkrbn_core_configuration* p);
const char* libkrbn_core_configuration_get_save_error_message(void);

// global_configuration

bool libkrbn_core_configuration_get_global_configuration_check_for_updates_on_startup(libkrbn_core_configuration* p);
void libkrbn_core_configuration_set_global_configuration_check_for_updates_on_startup(libkrbn_core_configuration* p, bool value);
bool libkrbn_core_configuration_get_global_configuration_show_in_menu_bar(libkrbn_core_configuration* p);
void libkrbn_core_configuration_set_global_configuration_show_in_menu_bar(libkrbn_core_configuration* p, bool value);
bool libkrbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar(libkrbn_core_configuration* p);
void libkrbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(libkrbn_core_configuration* p, bool value);
bool libkrbn_core_configuration_get_global_configuration_ask_for_confirmation_before_quitting(libkrbn_core_configuration* p);
void libkrbn_core_configuration_set_global_configuration_ask_for_confirmation_before_quitting(libkrbn_core_configuration* p, bool value);
bool libkrbn_core_configuration_get_global_configuration_unsafe_ui(libkrbn_core_configuration* p);
void libkrbn_core_configuration_set_global_configuration_unsafe_ui(libkrbn_core_configuration* p, bool value);

// profiles

size_t libkrbn_core_configuration_get_profiles_size(libkrbn_core_configuration* p);
const char* libkrbn_core_configuration_get_profile_name(libkrbn_core_configuration* p, size_t index);
void libkrbn_core_configuration_set_profile_name(libkrbn_core_configuration* p, size_t index, const char* value);
bool libkrbn_core_configuration_get_profile_selected(libkrbn_core_configuration* p, size_t index);
void libkrbn_core_configuration_select_profile(libkrbn_core_configuration* p, size_t index);
const char* libkrbn_core_configuration_get_selected_profile_name(libkrbn_core_configuration* p);
void libkrbn_core_configuration_push_back_profile(libkrbn_core_configuration* p);
void libkrbn_core_configuration_duplicate_profile(libkrbn_core_configuration* p, size_t source_index);
void libkrbn_core_configuration_erase_profile(libkrbn_core_configuration* p, size_t index);

// profile::parameters

int libkrbn_core_configuration_get_selected_profile_parameters_delay_milliseconds_before_open_device(libkrbn_core_configuration* p);
void libkrbn_core_configuration_set_selected_profile_parameters_delay_milliseconds_before_open_device(libkrbn_core_configuration* p, int value);

// profile::simple_modifications

size_t libkrbn_core_configuration_get_selected_profile_simple_modifications_size(libkrbn_core_configuration* p,
                                                                                 const libkrbn_device_identifiers* device_identifiers);
const char* libkrbn_core_configuration_get_selected_profile_simple_modification_from_json_string(libkrbn_core_configuration* p,
                                                                                                 size_t index,
                                                                                                 const libkrbn_device_identifiers* device_identifiers);
const char* libkrbn_core_configuration_get_selected_profile_simple_modification_to_json_string(libkrbn_core_configuration* p,
                                                                                               size_t index,
                                                                                               const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_replace_selected_profile_simple_modification(libkrbn_core_configuration* p,
                                                                             size_t index,
                                                                             const char* from_json_string,
                                                                             const char* to_json_string,
                                                                             const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_push_back_selected_profile_simple_modification(libkrbn_core_configuration* p,
                                                                               const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_erase_selected_profile_simple_modification(libkrbn_core_configuration* p, size_t index,
                                                                           const libkrbn_device_identifiers* device_identifiers);

// profile::fn_function_keys

size_t libkrbn_core_configuration_get_selected_profile_fn_function_keys_size(libkrbn_core_configuration* p,
                                                                             const libkrbn_device_identifiers* device_identifiers);
const char* libkrbn_core_configuration_get_selected_profile_fn_function_key_from_json_string(libkrbn_core_configuration* p,
                                                                                             size_t index,
                                                                                             const libkrbn_device_identifiers* device_identifiers);
const char* libkrbn_core_configuration_get_selected_profile_fn_function_key_to_json_string(libkrbn_core_configuration* p,
                                                                                           size_t index,
                                                                                           const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_replace_selected_profile_fn_function_key(libkrbn_core_configuration* p,
                                                                         const char* from_json_string,
                                                                         const char* to_json_string,
                                                                         const libkrbn_device_identifiers* device_identifiers);

// profile:complex_modifications

size_t libkrbn_core_configuration_get_selected_profile_complex_modifications_rules_size(libkrbn_core_configuration* p);
const char* libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_description(libkrbn_core_configuration* p, size_t index);
bool libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_json_string(libkrbn_core_configuration* p,
                                                                                            size_t index,
                                                                                            char* buffer,
                                                                                            size_t length);
void libkrbn_core_configuration_replace_selected_profile_complex_modifications_rule(libkrbn_core_configuration* p,
                                                                                    size_t index,
                                                                                    const char* json_string,
                                                                                    char* error_message_buffer,
                                                                                    size_t error_message_buffer_length);
void libkrbn_core_configuration_push_front_selected_profile_complex_modifications_rule(libkrbn_core_configuration* p,
                                                                                       const char* json_string,
                                                                                       char* error_message_buffer,
                                                                                       size_t error_message_buffer_length);
void libkrbn_core_configuration_erase_selected_profile_complex_modifications_rule(libkrbn_core_configuration* p, size_t index);
void libkrbn_core_configuration_swap_selected_profile_complex_modifications_rules(libkrbn_core_configuration* p, size_t index1, size_t index2);
void libkrbn_core_configuration_move_selected_profile_complex_modifications_rule(libkrbn_core_configuration* p, size_t source_index, size_t destination_index);
int libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(libkrbn_core_configuration* p,
                                                                                    const char* name);
void libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter(libkrbn_core_configuration* p,
                                                                                     const char* name,
                                                                                     int value);
void libkrbn_core_configuration_get_new_complex_modifications_rule_json_string(char* buffer,
                                                                               size_t length);

// profile::virtual_hid_device

uint8_t libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_country_code(libkrbn_core_configuration* p);
void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_country_code(libkrbn_core_configuration* p, uint8_t value);

int libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(libkrbn_core_configuration* p);
void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(libkrbn_core_configuration* p, int value);

bool libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(libkrbn_core_configuration* p);
void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(libkrbn_core_configuration* p, bool value);

// profile::devices

bool libkrbn_core_configuration_get_selected_profile_device_ignore(libkrbn_core_configuration* p,
                                                                   const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_ignore(libkrbn_core_configuration* p,
                                                                   const libkrbn_device_identifiers* device_identifiers,
                                                                   bool value);
bool libkrbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led(libkrbn_core_configuration* p,
                                                                                     const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_manipulate_caps_lock_led(libkrbn_core_configuration* p,
                                                                                     const libkrbn_device_identifiers* device_identifiers,
                                                                                     bool value);
bool libkrbn_core_configuration_get_selected_profile_device_treat_as_built_in_keyboard(libkrbn_core_configuration* p,
                                                                                       const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_treat_as_built_in_keyboard(libkrbn_core_configuration* p,
                                                                                       const libkrbn_device_identifiers* device_identifiers,
                                                                                       bool value);
bool libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(libkrbn_core_configuration* p,
                                                                                                const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(libkrbn_core_configuration* p,
                                                                                                const libkrbn_device_identifiers* device_identifiers,
                                                                                                bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_flip_x(libkrbn_core_configuration* p,
                                                                         const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_flip_x(libkrbn_core_configuration* p,
                                                                         const libkrbn_device_identifiers* device_identifiers,
                                                                         bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_flip_y(libkrbn_core_configuration* p,
                                                                         const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_flip_y(libkrbn_core_configuration* p,
                                                                         const libkrbn_device_identifiers* device_identifiers,
                                                                         bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_flip_vertical_wheel(libkrbn_core_configuration* p,
                                                                                      const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_flip_vertical_wheel(libkrbn_core_configuration* p,
                                                                                      const libkrbn_device_identifiers* device_identifiers,
                                                                                      bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_flip_horizontal_wheel(libkrbn_core_configuration* p,
                                                                                        const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_flip_horizontal_wheel(libkrbn_core_configuration* p,
                                                                                        const libkrbn_device_identifiers* device_identifiers,
                                                                                        bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_swap_xy(libkrbn_core_configuration* p,
                                                                          const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_swap_xy(libkrbn_core_configuration* p,
                                                                          const libkrbn_device_identifiers* device_identifiers,
                                                                          bool value);
bool libkrbn_core_configuration_get_selected_profile_device_mouse_swap_wheels(libkrbn_core_configuration* p,
                                                                              const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_mouse_swap_wheels(libkrbn_core_configuration* p,
                                                                              const libkrbn_device_identifiers* device_identifiers,
                                                                              bool value);
bool libkrbn_core_configuration_get_selected_profile_device_game_pad_swap_sticks(libkrbn_core_configuration* p,
                                                                                 const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_swap_sticks(libkrbn_core_configuration* p,
                                                                                 const libkrbn_device_identifiers* device_identifiers,
                                                                                 bool value);

bool libkrbn_core_configuration_has_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(libkrbn_core_configuration* p,
                                                                                                                              const libkrbn_device_identifiers* device_identifiers);
double libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(libkrbn_core_configuration* p,
                                                                                                                                const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(libkrbn_core_configuration* p,
                                                                                                                              const libkrbn_device_identifiers* device_identifiers,
                                                                                                                              double value);
void libkrbn_core_configuration_unset_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(libkrbn_core_configuration* p,
                                                                                                                                const libkrbn_device_identifiers* device_identifiers);

bool libkrbn_core_configuration_has_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(libkrbn_core_configuration* p,
                                                                                                                                  const libkrbn_device_identifiers* device_identifiers);
double libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(libkrbn_core_configuration* p,
                                                                                                                                    const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(libkrbn_core_configuration* p,
                                                                                                                                  const libkrbn_device_identifiers* device_identifiers,
                                                                                                                                  double value);
void libkrbn_core_configuration_unset_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(libkrbn_core_configuration* p,
                                                                                                                                    const libkrbn_device_identifiers* device_identifiers);

//
// libkrbn_complex_modifications_assets_manager
//

void libkrbn_enable_complex_modifications_assets_manager(void);
void libkrbn_disable_complex_modifications_assets_manager(void);

void libkrbn_complex_modifications_assets_manager_reload(void);

size_t libkrbn_complex_modifications_assets_manager_get_files_size(void);
const char* libkrbn_complex_modifications_assets_manager_get_file_title(size_t index);
time_t libkrbn_complex_modifications_assets_manager_get_file_last_write_time(size_t index);

size_t libkrbn_complex_modifications_assets_manager_get_rules_size(size_t file_index);
const char* libkrbn_complex_modifications_assets_manager_get_rule_description(size_t file_index,
                                                                              size_t index);

void libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(size_t file_index,
                                                                                                  size_t index,
                                                                                                  libkrbn_core_configuration* core_configuration);
bool libkrbn_complex_modifications_assets_manager_user_file(size_t index);
void libkrbn_complex_modifications_assets_manager_erase_file(size_t index);

//
// libkrbn_configuration_monitor
//

// You have to call `libkrbn_core_configuration_terminate(&initialized_core_configuration)`.
typedef void (*libkrbn_configuration_monitor_callback)(libkrbn_core_configuration* initialized_core_configuration,
                                                       void* refcon);
void libkrbn_enable_configuration_monitor(libkrbn_configuration_monitor_callback callback,
                                          void* refcon);
void libkrbn_disable_configuration_monitor(void);

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

typedef void libkrbn_connected_devices;
void libkrbn_connected_devices_terminate(libkrbn_connected_devices** p);

size_t libkrbn_connected_devices_get_size(libkrbn_connected_devices* p);
const char* libkrbn_connected_devices_get_descriptions_manufacturer(libkrbn_connected_devices* p, size_t index);
const char* libkrbn_connected_devices_get_descriptions_product(libkrbn_connected_devices* p, size_t index);
const char* libkrbn_connected_devices_get_descriptions_transport(libkrbn_connected_devices* p, size_t index);
bool libkrbn_connected_devices_get_device_identifiers(libkrbn_connected_devices* p,
                                                      size_t index,
                                                      libkrbn_device_identifiers* device_identifiers);
uint64_t libkrbn_connected_devices_get_vendor_id(libkrbn_connected_devices* p, size_t index);
uint64_t libkrbn_connected_devices_get_product_id(libkrbn_connected_devices* p, size_t index);
const char* libkrbn_connected_devices_get_device_address(libkrbn_connected_devices* p, size_t index);
bool libkrbn_connected_devices_get_is_keyboard(libkrbn_connected_devices* p, size_t index);
bool libkrbn_connected_devices_get_is_pointing_device(libkrbn_connected_devices* p, size_t index);
bool libkrbn_connected_devices_get_is_game_pad(libkrbn_connected_devices* p, size_t index);
bool libkrbn_connected_devices_get_is_built_in_keyboard(libkrbn_connected_devices* p, size_t index);
bool libkrbn_connected_devices_get_is_built_in_trackpad(libkrbn_connected_devices* p, size_t index);
bool libkrbn_connected_devices_get_is_built_in_touch_bar(libkrbn_connected_devices* p, size_t index);
bool libkrbn_connected_devices_is_apple(libkrbn_connected_devices* p, size_t index);

// You have to call `libkrbn_connected_devices_terminate(&initialized_connected_devices)`.
typedef void (*libkrbn_connected_devices_monitor_callback)(libkrbn_connected_devices* initialized_connected_devices,
                                                           void* refcon);
void libkrbn_enable_connected_devices_monitor(libkrbn_connected_devices_monitor_callback callback,
                                              void* refcon);
void libkrbn_disable_connected_devices_monitor(void);

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

const char* libkrbn_get_notification_message_body(void);

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

typedef void libkrbn_log_lines;
typedef void (*libkrbn_log_monitor_callback)(libkrbn_log_lines* log_lines, void* refcon);
void libkrbn_enable_log_monitor(libkrbn_log_monitor_callback callback,
                                void* refcon);
void libkrbn_disable_log_monitor(void);

size_t libkrbn_log_lines_get_size(libkrbn_log_lines* p);
const char* libkrbn_log_lines_get_line(libkrbn_log_lines* p, size_t index);
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
