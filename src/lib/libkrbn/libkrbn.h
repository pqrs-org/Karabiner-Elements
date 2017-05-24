#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
  
struct vendor_product_pair {
  uint32_t vendor_id;
  uint32_t product_id;
};

void libkrbn_initialize(void);

const char* _Nonnull libkrbn_get_distributed_notification_observed_object(void);
const char* _Nonnull libkrbn_get_distributed_notification_grabber_is_launched(void);
const char* _Nonnull libkrbn_get_distributed_notification_console_user_server_is_disabled(void);
const char* _Nonnull libkrbn_get_devices_json_file_path(void);

bool libkrbn_lock_single_application_with_user_pid_file(const char* _Nonnull pid_file_name);
void libkrbn_unlock_single_application(void);

bool libkrbn_save_beautified_json_string(const char* _Nonnull file_path, const char* _Nonnull json_string);

void libkrbn_launchctl_manage_console_user_server(bool load);

void libkrbn_check_for_updates_in_background(void);
void libkrbn_check_for_updates_stable_only(void);
void libkrbn_check_for_updates_with_beta_version(void);

void libkrbn_launch_event_viewer(void);
void libkrbn_launch_menu(void);
void libkrbn_launch_preferences(void);

bool libkrbn_system_core_configuration_file_path_exists(void);

// ----------------------------------------
// libkrbn_core_configuration

typedef void libkrbn_core_configuration;
void libkrbn_core_configuration_terminate(libkrbn_core_configuration* _Nullable* _Nonnull p);
bool libkrbn_core_configuration_save(libkrbn_core_configuration* _Nonnull p);

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

size_t libkrbn_core_configuration_get_selected_profile_simple_modifications_size(libkrbn_core_configuration* _Nonnull p);
const char* _Nullable libkrbn_core_configuration_get_selected_profile_simple_modification_first(libkrbn_core_configuration* _Nonnull p, size_t index);
const char* _Nullable libkrbn_core_configuration_get_selected_profile_simple_modification_second(libkrbn_core_configuration* _Nonnull p, size_t index);
void libkrbn_core_configuration_replace_selected_profile_simple_modification(libkrbn_core_configuration* _Nonnull p,
                                                                             size_t index,
                                                                             const char* _Nonnull from,
                                                                             const char* _Nonnull to);
  
void libkrbn_core_configuration_replace_selected_profile_simple_modification_vendor_product_id(libkrbn_core_configuration* _Nonnull p,
                                                                                            size_t index,
                                                                                            uint32_t vendorId,
                                                                                            uint32_t productId);
  
const uint32_t libkrbn_core_configuration_get_selected_profile_simple_modification_vendor_id(libkrbn_core_configuration* _Nonnull p, size_t index);
const uint32_t libkrbn_core_configuration_get_selected_profile_simple_modification_product_id(libkrbn_core_configuration* _Nonnull p, size_t index);
const bool libkrbn_core_configuration_get_selected_profile_simple_modification_disabled(libkrbn_core_configuration* _Nonnull p, size_t index);
struct vendor_product_pair* _Nullable libkrbn_core_configuration_get_selected_profile_simple_modification_vendor_product_pairs(libkrbn_core_configuration* _Nonnull p, size_t* _Nonnull count);
void libkrbn_core_configuration_push_back_selected_profile_simple_modification(libkrbn_core_configuration* _Nonnull p);
void libkrbn_core_configuration_erase_selected_profile_simple_modification(libkrbn_core_configuration* _Nonnull p, size_t index);

// profile::fn_function_keys

size_t libkrbn_core_configuration_get_selected_profile_fn_function_keys_size(libkrbn_core_configuration* _Nonnull p);
const char* _Nullable libkrbn_core_configuration_get_selected_profile_fn_function_key_first(libkrbn_core_configuration* _Nonnull p, size_t index);
const char* _Nullable libkrbn_core_configuration_get_selected_profile_fn_function_key_second(libkrbn_core_configuration* _Nonnull p, size_t index);
void libkrbn_core_configuration_replace_selected_profile_fn_function_key(libkrbn_core_configuration* _Nonnull p,
                                                                         const char* _Nonnull from,
                                                                         const char* _Nonnull to);

// profile::virtual_hid_device

const char* _Nullable libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_keyboard_type(libkrbn_core_configuration* _Nonnull p);
void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_keyboard_type(libkrbn_core_configuration* _Nonnull p, const char* _Nonnull value);
int libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_caps_lock_delay_milliseconds(libkrbn_core_configuration* _Nonnull p);
void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_caps_lock_delay_milliseconds(libkrbn_core_configuration* _Nonnull p, uint32_t value);

// profile::devices

bool libkrbn_core_configuration_get_selected_profile_device_ignore(libkrbn_core_configuration* _Nonnull p,
                                                                   uint32_t vendor_id,
                                                                   uint32_t product_id,
                                                                   bool is_keyboard,
                                                                   bool is_pointing_device);
void libkrbn_core_configuration_set_selected_profile_device_ignore(libkrbn_core_configuration* _Nonnull p,
                                                                   uint32_t vendor_id,
                                                                   uint32_t product_id,
                                                                   bool is_keyboard,
                                                                   bool is_pointing_device,
                                                                   bool value);
bool libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(libkrbn_core_configuration* _Nonnull p,
                                                                                                uint32_t vendor_id,
                                                                                                uint32_t product_id,
                                                                                                bool is_keyboard,
                                                                                                bool is_pointing_device);
void libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(libkrbn_core_configuration* _Nonnull p,
                                                                                                uint32_t vendor_id,
                                                                                                uint32_t product_id,
                                                                                                bool is_keyboard,
                                                                                                bool is_pointing_device,
                                                                                                bool value);

void libkrbn_core_configuration_get_selected_profile_device_product_manufacturer(libkrbn_core_configuration* _Nonnull p,
                                                                                                  uint32_t vendor_id,
                                                                                                  uint32_t product_id,
                                                                                                  const char * _Nonnull * _Nullable product,
                                                                                                  const char * _Nonnull * _Nullable manufacturer);

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

struct libkrbn_system_preferences_values {
  bool keyboard_fn_state;
};

typedef void libkrbn_system_preferences_monitor;
typedef void (*libkrbn_system_preferences_monitor_callback)(const struct libkrbn_system_preferences_values* _Nonnull system_preferences_values,
                                                            void* _Nullable refcon);
bool libkrbn_system_preferences_monitor_initialize(libkrbn_system_preferences_monitor* _Nullable* _Nonnull out,
                                                   libkrbn_system_preferences_monitor_callback _Nullable callback,
                                                   void* _Nullable refcon);
void libkrbn_system_preferences_monitor_terminate(libkrbn_system_preferences_monitor* _Nullable* _Nonnull p);

// ----------------------------------------
// libkrbn_connected_devices

typedef void libkrbn_connected_devices;
void libkrbn_connected_devices_terminate(libkrbn_connected_devices* _Nullable* _Nonnull p);

size_t libkrbn_connected_devices_get_size(libkrbn_connected_devices* _Nonnull p);
const char* _Nullable libkrbn_connected_devices_get_descriptions_manufacturer(libkrbn_connected_devices* _Nonnull p, size_t index);
const char* _Nullable libkrbn_connected_devices_get_descriptions_product(libkrbn_connected_devices* _Nonnull p, size_t index);
uint32_t libkrbn_connected_devices_get_identifiers_vendor_id(libkrbn_connected_devices* _Nonnull p, size_t index);
uint32_t libkrbn_connected_devices_get_identifiers_product_id(libkrbn_connected_devices* _Nonnull p, size_t index);
bool libkrbn_connected_devices_get_identifiers_is_keyboard(libkrbn_connected_devices* _Nonnull p, size_t index);
bool libkrbn_connected_devices_get_identifiers_is_pointing_device(libkrbn_connected_devices* _Nonnull p, size_t index);
bool libkrbn_connected_devices_get_is_built_in_keyboard(libkrbn_connected_devices* _Nonnull p, size_t index);

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

// ----------------------------------------
// libkrbn_version_monitor

typedef void libkrbn_version_monitor;
typedef void (*libkrbn_version_monitor_callback)(void* _Nullable refcon);
bool libkrbn_version_monitor_initialize(libkrbn_version_monitor* _Nullable* _Nonnull out,
                                        libkrbn_version_monitor_callback _Nullable callback,
                                        void* _Nullable refcon);
void libkrbn_version_monitor_terminate(libkrbn_version_monitor* _Nullable* _Nonnull p);

#ifdef __cplusplus
}
#endif
