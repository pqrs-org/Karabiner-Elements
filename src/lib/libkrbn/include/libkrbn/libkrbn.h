#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t vendor_id;
  uint32_t product_id;
  bool is_keyboard;
  bool is_pointing_device;
} libkrbn_device_identifiers;

void libkrbn_initialize(void);
void libkrbn_terminate(void);

const char* libkrbn_get_distributed_notification_observed_object(void);
const char* libkrbn_get_distributed_notification_console_user_server_is_disabled(void);
const char* libkrbn_get_distributed_notification_device_grabbing_state_is_changed(void);
const char* libkrbn_get_devices_json_file_path(void);
const char* libkrbn_get_user_complex_modifications_assets_directory(void);

bool libkrbn_lock_single_application_with_user_pid_file(const char* pid_file_name);
void libkrbn_unlock_single_application(void);

void libkrbn_launchctl_manage_console_user_server(bool load);
void libkrbn_launchctl_restart_console_user_server(void);

void libkrbn_check_for_updates_in_background(void);
void libkrbn_check_for_updates_stable_only(void);
void libkrbn_check_for_updates_with_beta_version(void);

void libkrbn_launch_event_viewer(void);
void libkrbn_launch_menu(void);
void libkrbn_launch_preferences(void);

bool libkrbn_system_core_configuration_file_path_exists(void);

// types

void libkrbn_get_key_code_name(char* buffer, size_t length, uint32_t key_code);
void libkrbn_get_consumer_key_code_name(char* buffer, size_t length, uint32_t consumer_key_code);

// device_identifiers

bool libkrbn_device_identifiers_is_apple(const libkrbn_device_identifiers* p);

// ----------------------------------------
// libkrbn_core_configuration

typedef void libkrbn_core_configuration;
void libkrbn_core_configuration_terminate(libkrbn_core_configuration** p);
void libkrbn_core_configuration_save(libkrbn_core_configuration* p);

// global_configuration

bool libkrbn_core_configuration_get_global_configuration_check_for_updates_on_startup(libkrbn_core_configuration* p);
void libkrbn_core_configuration_set_global_configuration_check_for_updates_on_startup(libkrbn_core_configuration* p, bool value);
bool libkrbn_core_configuration_get_global_configuration_show_in_menu_bar(libkrbn_core_configuration* p);
void libkrbn_core_configuration_set_global_configuration_show_in_menu_bar(libkrbn_core_configuration* p, bool value);
bool libkrbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar(libkrbn_core_configuration* p);
void libkrbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(libkrbn_core_configuration* p, bool value);

// profiles

size_t libkrbn_core_configuration_get_profiles_size(libkrbn_core_configuration* p);
const char* libkrbn_core_configuration_get_profile_name(libkrbn_core_configuration* p, size_t index);
void libkrbn_core_configuration_set_profile_name(libkrbn_core_configuration* p, size_t index, const char* value);
bool libkrbn_core_configuration_get_profile_selected(libkrbn_core_configuration* p, size_t index);
void libkrbn_core_configuration_select_profile(libkrbn_core_configuration* p, size_t index);
const char* libkrbn_core_configuration_get_selected_profile_name(libkrbn_core_configuration* p);
void libkrbn_core_configuration_push_back_profile(libkrbn_core_configuration* p);
void libkrbn_core_configuration_erase_profile(libkrbn_core_configuration* p, size_t index);

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
void libkrbn_core_configuration_erase_selected_profile_complex_modifications_rule(libkrbn_core_configuration* p, size_t index);
void libkrbn_core_configuration_swap_selected_profile_complex_modifications_rules(libkrbn_core_configuration* p, size_t index1, size_t index2);
int libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(libkrbn_core_configuration* p,
                                                                                    const char* name);
void libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter(libkrbn_core_configuration* p,
                                                                                     const char* name,
                                                                                     int value);

// profile::virtual_hid_device

uint8_t libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_country_code(libkrbn_core_configuration* p);
void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_country_code(libkrbn_core_configuration* p, uint8_t value);

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
bool libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(libkrbn_core_configuration* p,
                                                                                                const libkrbn_device_identifiers* device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(libkrbn_core_configuration* p,
                                                                                                const libkrbn_device_identifiers* device_identifiers,
                                                                                                bool value);

// ----------------------------------------
// libkrbn_complex_modifications_assets_manager

typedef void libkrbn_complex_modifications_assets_manager;

bool libkrbn_complex_modifications_assets_manager_initialize(libkrbn_complex_modifications_assets_manager** out);
void libkrbn_complex_modifications_assets_manager_terminate(libkrbn_complex_modifications_assets_manager** p);

void libkrbn_complex_modifications_assets_manager_reload(libkrbn_complex_modifications_assets_manager* p);

size_t libkrbn_complex_modifications_assets_manager_get_files_size(libkrbn_complex_modifications_assets_manager* p);
const char* libkrbn_complex_modifications_assets_manager_get_file_title(libkrbn_complex_modifications_assets_manager* p,
                                                                        size_t index);

size_t libkrbn_complex_modifications_assets_manager_get_file_rules_size(libkrbn_complex_modifications_assets_manager* p,
                                                                        size_t file_index);
const char* libkrbn_complex_modifications_assets_manager_get_file_rule_description(libkrbn_complex_modifications_assets_manager* p,
                                                                                   size_t file_index,
                                                                                   size_t index);

void libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(libkrbn_complex_modifications_assets_manager* p,
                                                                                                  size_t file_index,
                                                                                                  size_t index,
                                                                                                  libkrbn_core_configuration* q);
bool libkrbn_complex_modifications_assets_manager_is_user_file(libkrbn_complex_modifications_assets_manager* p,
                                                               size_t index);
void libkrbn_complex_modifications_assets_manager_erase_file(libkrbn_complex_modifications_assets_manager* p,
                                                             size_t index);

// ----------------------------------------
// libkrbn_configuration_monitor

typedef void libkrbn_configuration_monitor;
// You have to call `libkrbn_core_configuration_terminate(&initialized_core_configuration)`.
typedef void (*libkrbn_configuration_monitor_callback)(libkrbn_core_configuration* initialized_core_configuration,
                                                       void* refcon);
bool libkrbn_configuration_monitor_initialize(libkrbn_configuration_monitor** out,
                                              libkrbn_configuration_monitor_callback callback,
                                              void* refcon);
void libkrbn_configuration_monitor_terminate(libkrbn_configuration_monitor** p);

// ----------------------------------------
// libkrbn_system_preferences_monitor

struct libkrbn_system_preferences {
  bool keyboard_fn_state;
};

typedef void (*libkrbn_system_preferences_monitor_callback)(const struct libkrbn_system_preferences* system_preferences,
                                                            void* refcon);
void libkrbn_enable_system_preferences_monitor(libkrbn_system_preferences_monitor_callback callback,
                                               void* refcon);
void libkrbn_disable_system_preferences_monitor(void);

// ----------------------------------------
// libkrbn_connected_devices

typedef void libkrbn_connected_devices;
void libkrbn_connected_devices_terminate(libkrbn_connected_devices** p);

size_t libkrbn_connected_devices_get_size(libkrbn_connected_devices* p);
const char* libkrbn_connected_devices_get_descriptions_manufacturer(libkrbn_connected_devices* p, size_t index);
const char* libkrbn_connected_devices_get_descriptions_product(libkrbn_connected_devices* p, size_t index);
bool libkrbn_connected_devices_get_device_identifiers(libkrbn_connected_devices* p,
                                                      size_t index,
                                                      libkrbn_device_identifiers* device_identifiers);
bool libkrbn_connected_devices_get_is_built_in_keyboard(libkrbn_connected_devices* p, size_t index);
bool libkrbn_connected_devices_get_is_built_in_trackpad(libkrbn_connected_devices* p, size_t index);

typedef void (*libkrbn_connected_devices_monitor_callback)(libkrbn_connected_devices* initialized_connected_devices,
                                                           void* refcon);
void libkrbn_enable_connected_devices_monitor(libkrbn_connected_devices_monitor_callback callback,
                                              void* refcon);
void libkrbn_disable_connected_devices_monitor(void);

// ----------------------------------------
// libkrbn_version_monitor

typedef void (*libkrbn_version_monitor_callback)(void* refcon);
void libkrbn_enable_version_monitor(libkrbn_version_monitor_callback callback,
                                    void* refcon);
void libkrbn_disable_version_monitor(void);

// ----------------------------------------
// libkrbn_file_monitor

typedef void (*libkrbn_file_monitor_callback)(const char* file_path,
                                              void* refcon);

// ----------------------------------------
// libkrbn_device_details_json_file_monitor

void libkrbn_enable_device_details_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                     void* refcon);
void libkrbn_disable_device_details_json_file_monitor(void);

// ----------------------------------------
// libkrbn_manipulator_environment_json_file_monitor

void libkrbn_enable_manipulator_environment_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                              void* refcon);
void libkrbn_disable_manipulator_environment_json_file_monitor(void);

// ----------------------------------------
// libkrbn_grabber_alerts_json_file_monitor

void libkrbn_enable_grabber_alerts_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                     void* refcon);
void libkrbn_disable_grabber_alerts_json_file_monitor(void);

// ----------------------------------------
// libkrbn_frontmost_application_monitor

typedef void (*libkrbn_frontmost_application_monitor_callback)(const char* bundle_identifier,
                                                               const char* file_path,
                                                               void* refcon);
void libkrbn_enable_frontmost_application_monitor(libkrbn_frontmost_application_monitor_callback callback,
                                                  void* refcon);
void libkrbn_disable_frontmost_application_monitor(void);

// ----------------------------------------
// libkrbn_log_monitor

typedef void libkrbn_log_lines;
typedef void (*libkrbn_log_monitor_callback)(libkrbn_log_lines* log_lines, void* refcon);
void libkrbn_enable_log_monitor(libkrbn_log_monitor_callback callback,
                                void* refcon);
void libkrbn_disable_log_monitor(void);

size_t libkrbn_log_lines_get_size(libkrbn_log_lines* p);
const char* libkrbn_log_lines_get_line(libkrbn_log_lines* p, size_t index);
bool libkrbn_log_lines_is_warn_line(const char* line);
bool libkrbn_log_lines_is_error_line(const char* line);

// ----------------------------------------
// libkrbn_hid_value_monitor

enum libkrbn_hid_value_type {
  libkrbn_hid_value_type_key_code,
  libkrbn_hid_value_type_consumer_key_code,
};

enum libkrbn_hid_value_event_type {
  libkrbn_hid_value_event_type_key_down,
  libkrbn_hid_value_event_type_key_up,
  libkrbn_hid_value_event_type_single,
};

typedef void (*libkrbn_hid_value_monitor_callback)(enum libkrbn_hid_value_type type,
                                                   uint32_t value,
                                                   enum libkrbn_hid_value_event_type event_type,
                                                   void* refcon);
void libkrbn_enable_hid_value_monitor(libkrbn_hid_value_monitor_callback callback,
                                      void* refcon);
void libkrbn_disable_hid_value_monitor(void);
bool libkrbn_hid_value_monitor_observed(void);

#ifdef __cplusplus
}
#endif
