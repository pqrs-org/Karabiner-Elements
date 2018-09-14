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

const char* _Nonnull libkrbn_get_distributed_notification_observed_object(void);
const char* _Nonnull libkrbn_get_distributed_notification_grabber_is_launched(void);
const char* _Nonnull libkrbn_get_distributed_notification_console_user_server_is_disabled(void);
const char* _Nonnull libkrbn_get_distributed_notification_device_grabbing_state_is_changed(void);
const char* _Nonnull libkrbn_get_grabber_alerts_json_file_path(void);
const char* _Nonnull libkrbn_get_devices_json_file_path(void);
const char* _Nonnull libkrbn_get_device_details_json_file_path(void);
const char* _Nonnull libkrbn_get_manipulator_environment_json_file_path(void);
const char* _Nonnull libkrbn_get_user_complex_modifications_assets_directory(void);

bool libkrbn_lock_single_application_with_user_pid_file(const char* _Nonnull pid_file_name);
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

void libkrbn_get_key_code_name(char* _Nonnull buffer, size_t length, uint32_t key_code);
void libkrbn_get_consumer_key_code_name(char* _Nonnull buffer, size_t length, uint32_t consumer_key_code);

// device_identifiers

bool libkrbn_device_identifiers_is_apple(const libkrbn_device_identifiers* _Nonnull p);

// ----------------------------------------
// libkrbn_core_configuration

typedef void libkrbn_core_configuration;
void libkrbn_core_configuration_terminate(libkrbn_core_configuration* _Nullable* _Nonnull p);
void libkrbn_core_configuration_save(libkrbn_core_configuration* _Nonnull p);

// global_configuration

bool libkrbn_core_configuration_get_global_configuration_check_for_updates_on_startup(libkrbn_core_configuration* _Nonnull p);
void libkrbn_core_configuration_set_global_configuration_check_for_updates_on_startup(libkrbn_core_configuration* _Nonnull p, bool value);
bool libkrbn_core_configuration_get_global_configuration_show_in_menu_bar(libkrbn_core_configuration* _Nonnull p);
void libkrbn_core_configuration_set_global_configuration_show_in_menu_bar(libkrbn_core_configuration* _Nonnull p, bool value);
bool libkrbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar(libkrbn_core_configuration* _Nonnull p);
void libkrbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(libkrbn_core_configuration* _Nonnull p, bool value);

// profiles

size_t libkrbn_core_configuration_get_profiles_size(libkrbn_core_configuration* _Nonnull p);
const char* _Nullable libkrbn_core_configuration_get_profile_name(libkrbn_core_configuration* _Nonnull p, size_t index);
void libkrbn_core_configuration_set_profile_name(libkrbn_core_configuration* _Nonnull p, size_t index, const char* _Nonnull value);
bool libkrbn_core_configuration_get_profile_selected(libkrbn_core_configuration* _Nonnull p, size_t index);
void libkrbn_core_configuration_select_profile(libkrbn_core_configuration* _Nonnull p, size_t index);
const char* _Nullable libkrbn_core_configuration_get_selected_profile_name(libkrbn_core_configuration* _Nonnull p);
void libkrbn_core_configuration_push_back_profile(libkrbn_core_configuration* _Nonnull p);
void libkrbn_core_configuration_erase_profile(libkrbn_core_configuration* _Nonnull p, size_t index);

// profile::simple_modifications

size_t libkrbn_core_configuration_get_selected_profile_simple_modifications_size(libkrbn_core_configuration* _Nonnull p,
                                                                                 const libkrbn_device_identifiers* _Nullable device_identifiers);
const char* _Nullable libkrbn_core_configuration_get_selected_profile_simple_modification_from_json_string(libkrbn_core_configuration* _Nonnull p,
                                                                                                           size_t index,
                                                                                                           const libkrbn_device_identifiers* _Nullable device_identifiers);
const char* _Nullable libkrbn_core_configuration_get_selected_profile_simple_modification_to_json_string(libkrbn_core_configuration* _Nonnull p,
                                                                                                         size_t index,
                                                                                                         const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_replace_selected_profile_simple_modification(libkrbn_core_configuration* _Nonnull p,
                                                                             size_t index,
                                                                             const char* _Nonnull from_json_string,
                                                                             const char* _Nonnull to_json_string,
                                                                             const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_push_back_selected_profile_simple_modification(libkrbn_core_configuration* _Nonnull p,
                                                                               const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_erase_selected_profile_simple_modification(libkrbn_core_configuration* _Nonnull p, size_t index,
                                                                           const libkrbn_device_identifiers* _Nullable device_identifiers);

// profile::fn_function_keys

size_t libkrbn_core_configuration_get_selected_profile_fn_function_keys_size(libkrbn_core_configuration* _Nonnull p,
                                                                             const libkrbn_device_identifiers* _Nullable device_identifiers);
const char* _Nullable libkrbn_core_configuration_get_selected_profile_fn_function_key_from_json_string(libkrbn_core_configuration* _Nonnull p,
                                                                                                       size_t index,
                                                                                                       const libkrbn_device_identifiers* _Nullable device_identifiers);
const char* _Nullable libkrbn_core_configuration_get_selected_profile_fn_function_key_to_json_string(libkrbn_core_configuration* _Nonnull p,
                                                                                                     size_t index,
                                                                                                     const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_replace_selected_profile_fn_function_key(libkrbn_core_configuration* _Nonnull p,
                                                                         const char* _Nonnull from_json_string,
                                                                         const char* _Nonnull to_json_string,
                                                                         const libkrbn_device_identifiers* _Nullable device_identifiers);

// profile:complex_modifications

size_t libkrbn_core_configuration_get_selected_profile_complex_modifications_rules_size(libkrbn_core_configuration* _Nonnull p);
const char* _Nullable libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_description(libkrbn_core_configuration* _Nonnull p, size_t index);
void libkrbn_core_configuration_erase_selected_profile_complex_modifications_rule(libkrbn_core_configuration* _Nonnull p, size_t index);
void libkrbn_core_configuration_swap_selected_profile_complex_modifications_rules(libkrbn_core_configuration* _Nonnull p, size_t index1, size_t index2);
int libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(libkrbn_core_configuration* _Nonnull p,
                                                                                    const char* _Nonnull name);
void libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter(libkrbn_core_configuration* _Nonnull p,
                                                                                     const char* _Nonnull name,
                                                                                     int value);

// profile::virtual_hid_device

uint8_t libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_country_code(libkrbn_core_configuration* _Nonnull p);
void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_country_code(libkrbn_core_configuration* _Nonnull p, uint8_t value);

// profile::devices

bool libkrbn_core_configuration_get_selected_profile_device_ignore(libkrbn_core_configuration* _Nonnull p,
                                                                   const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_ignore(libkrbn_core_configuration* _Nonnull p,
                                                                   const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                   bool value);
bool libkrbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led(libkrbn_core_configuration* _Nonnull p,
                                                                                     const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_manipulate_caps_lock_led(libkrbn_core_configuration* _Nonnull p,
                                                                                     const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                     bool value);
bool libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(libkrbn_core_configuration* _Nonnull p,
                                                                                                const libkrbn_device_identifiers* _Nullable device_identifiers);
void libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(libkrbn_core_configuration* _Nonnull p,
                                                                                                const libkrbn_device_identifiers* _Nullable device_identifiers,
                                                                                                bool value);

// ----------------------------------------
// libkrbn_complex_modifications_assets_manager

typedef void libkrbn_complex_modifications_assets_manager;

bool libkrbn_complex_modifications_assets_manager_initialize(libkrbn_complex_modifications_assets_manager* _Nullable* _Nonnull out);
void libkrbn_complex_modifications_assets_manager_terminate(libkrbn_complex_modifications_assets_manager* _Nullable* _Nonnull p);

void libkrbn_complex_modifications_assets_manager_reload(libkrbn_complex_modifications_assets_manager* _Nonnull p);

size_t libkrbn_complex_modifications_assets_manager_get_files_size(libkrbn_complex_modifications_assets_manager* _Nonnull p);
const char* _Nullable libkrbn_complex_modifications_assets_manager_get_file_title(libkrbn_complex_modifications_assets_manager* _Nonnull p,
                                                                                  size_t index);

size_t libkrbn_complex_modifications_assets_manager_get_file_rules_size(libkrbn_complex_modifications_assets_manager* _Nonnull p,
                                                                        size_t file_index);
const char* _Nullable libkrbn_complex_modifications_assets_manager_get_file_rule_description(libkrbn_complex_modifications_assets_manager* _Nonnull p,
                                                                                             size_t file_index,
                                                                                             size_t index);

void libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(libkrbn_complex_modifications_assets_manager* _Nonnull p,
                                                                                                  size_t file_index,
                                                                                                  size_t index,
                                                                                                  libkrbn_core_configuration* _Nonnull q);
bool libkrbn_complex_modifications_assets_manager_is_user_file(libkrbn_complex_modifications_assets_manager* _Nonnull p,
                                                               size_t index);
void libkrbn_complex_modifications_assets_manager_erase_file(libkrbn_complex_modifications_assets_manager* _Nonnull p,
                                                             size_t index);

// ----------------------------------------
// libkrbn_configuration_monitor

typedef void libkrbn_configuration_monitor;
// You have to call `libkrbn_core_configuration_terminate(&initialized_core_configuration)`.
typedef void (*libkrbn_configuration_monitor_callback)(libkrbn_core_configuration* _Nonnull initialized_core_configuration,
                                                       void* _Nullable refcon);
bool libkrbn_configuration_monitor_initialize(libkrbn_configuration_monitor* _Nullable* _Nonnull out,
                                              libkrbn_configuration_monitor_callback _Nullable callback,
                                              void* _Nullable refcon);
void libkrbn_configuration_monitor_terminate(libkrbn_configuration_monitor* _Nullable* _Nonnull p);

// ----------------------------------------
// libkrbn_system_preferences_monitor

struct libkrbn_system_preferences {
  bool keyboard_fn_state;
};

typedef void libkrbn_system_preferences_monitor;
typedef void (*libkrbn_system_preferences_monitor_callback)(const struct libkrbn_system_preferences* _Nonnull system_preferences,
                                                            void* _Nullable refcon);
bool libkrbn_system_preferences_monitor_initialize(libkrbn_system_preferences_monitor* _Nullable* _Nonnull out,
                                                   libkrbn_system_preferences_monitor_callback _Nullable callback,
                                                   void* _Nullable refcon,
                                                   libkrbn_configuration_monitor* _Nonnull libkrbn_configuration_monitor);
void libkrbn_system_preferences_monitor_terminate(libkrbn_system_preferences_monitor* _Nullable* _Nonnull p);

// ----------------------------------------
// libkrbn_connected_devices

typedef void libkrbn_connected_devices;
void libkrbn_connected_devices_terminate(libkrbn_connected_devices* _Nullable* _Nonnull p);

size_t libkrbn_connected_devices_get_size(libkrbn_connected_devices* _Nonnull p);
const char* _Nullable libkrbn_connected_devices_get_descriptions_manufacturer(libkrbn_connected_devices* _Nonnull p, size_t index);
const char* _Nullable libkrbn_connected_devices_get_descriptions_product(libkrbn_connected_devices* _Nonnull p, size_t index);
bool libkrbn_connected_devices_get_device_identifiers(libkrbn_connected_devices* _Nonnull p,
                                                      size_t index,
                                                      libkrbn_device_identifiers* _Nonnull device_identifiers);
bool libkrbn_connected_devices_get_is_built_in_keyboard(libkrbn_connected_devices* _Nonnull p, size_t index);
bool libkrbn_connected_devices_get_is_built_in_trackpad(libkrbn_connected_devices* _Nonnull p, size_t index);

typedef void libkrbn_connected_devices_monitor;
typedef void (*libkrbn_connected_devices_monitor_callback)(libkrbn_connected_devices* _Nonnull initialized_connected_devices,
                                                           void* _Nullable refcon);
bool libkrbn_connected_devices_monitor_initialize(libkrbn_connected_devices_monitor* _Nullable* _Nonnull out,
                                                  libkrbn_connected_devices_monitor_callback _Nullable callback,
                                                  void* _Nullable refcon);
void libkrbn_connected_devices_monitor_terminate(libkrbn_connected_devices_monitor* _Nullable* _Nonnull p);

// ----------------------------------------
// libkrbn_log_monitor

typedef void libkrbn_log_monitor;
typedef void (*libkrbn_log_monitor_callback)(const char* _Nonnull line, void* _Nullable refcon);
bool libkrbn_log_monitor_initialize(libkrbn_log_monitor* _Nullable* _Nonnull out,
                                    libkrbn_log_monitor_callback _Nullable callback,
                                    void* _Nullable refcon);
void libkrbn_log_monitor_terminate(libkrbn_log_monitor* _Nullable* _Nonnull p);
size_t libkrbn_log_monitor_initial_lines_size(libkrbn_log_monitor* _Nonnull p);
const char* _Nullable libkrbn_log_monitor_initial_line(libkrbn_log_monitor* _Nonnull p, size_t index);
void libkrbn_log_monitor_start(libkrbn_log_monitor* _Nonnull p);
bool libkrbn_is_warn_log(const char* _Nonnull line);
bool libkrbn_is_err_log(const char* _Nonnull line);

// ----------------------------------------
// libkrbn_version_monitor

typedef void libkrbn_version_monitor;
typedef void (*libkrbn_version_monitor_callback)(void* _Nullable refcon);
bool libkrbn_version_monitor_initialize(libkrbn_version_monitor* _Nullable* _Nonnull out,
                                        libkrbn_version_monitor_callback _Nullable callback,
                                        void* _Nullable refcon);
void libkrbn_version_monitor_terminate(libkrbn_version_monitor* _Nullable* _Nonnull p);

// ----------------------------------------
// libkrbn_file_monitor

typedef void libkrbn_file_monitor;
typedef void (*libkrbn_file_monitor_callback)(void* _Nullable refcon);
bool libkrbn_file_monitor_initialize(libkrbn_file_monitor* _Nullable* _Nonnull out,
                                     const char* _Nonnull file_path,
                                     libkrbn_file_monitor_callback _Nullable callback,
                                     void* _Nullable refcon);
void libkrbn_file_monitor_terminate(libkrbn_file_monitor* _Nullable* _Nonnull p);

// ----------------------------------------
// libkrbn_hid_value_observer

enum libkrbn_hid_value_type {
  libkrbn_hid_value_type_key_code,
  libkrbn_hid_value_type_consumer_key_code,
};

enum libkrbn_hid_value_event_type {
  libkrbn_hid_value_event_type_key_down,
  libkrbn_hid_value_event_type_key_up,
  libkrbn_hid_value_event_type_single,
};

typedef void libkrbn_hid_value_observer;
typedef void (*libkrbn_hid_value_observer_callback)(enum libkrbn_hid_value_type type,
                                                    uint32_t value,
                                                    enum libkrbn_hid_value_event_type event_type,
                                                    void* _Nullable refcon);
bool libkrbn_hid_value_observer_initialize(libkrbn_hid_value_observer* _Nullable* _Nonnull out,
                                           libkrbn_hid_value_observer_callback _Nullable callback,
                                           void* _Nullable refcon);
void libkrbn_hid_value_observer_terminate(libkrbn_hid_value_observer* _Nullable* _Nonnull p);
size_t libkrbn_hid_value_observer_calculate_observed_device_count(libkrbn_hid_value_observer* _Nonnull p);

#ifdef __cplusplus
}
#endif
