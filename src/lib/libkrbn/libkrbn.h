#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void libkrbn_initialize(void);

const char* _Nonnull libkrbn_get_distributed_notification_observed_object(void);
const char* _Nonnull libkrbn_get_distributed_notification_grabber_is_launched(void);
const char* _Nonnull libkrbn_get_core_configuration_file_path(void);
const char* _Nonnull libkrbn_get_devices_json_file_path(void);

const char* _Nonnull libkrbn_get_default_profile_json_string(void);

bool libkrbn_save_beautified_json_string(const char* _Nonnull file_path, const char* _Nonnull json_string);

void libkrbn_launchctl_manage_console_user_server(bool load);

void libkrbn_check_for_updates_in_background(void);
void libkrbn_check_for_updates_stable_only(void);
void libkrbn_check_for_updates_with_beta_version(void);

// ----------------------------------------
// libkrbn_configuration_monitor

typedef void libkrbn_configuration_monitor;
typedef void (*libkrbn_configuration_monitor_callback)(const char* _Nonnull json, void* _Nullable refcon);
bool libkrbn_configuration_monitor_initialize(libkrbn_configuration_monitor* _Nullable* _Nonnull out,
                                              libkrbn_configuration_monitor_callback _Nullable callback,
                                              void* _Nullable refcon);
void libkrbn_configuration_monitor_terminate(libkrbn_configuration_monitor* _Nullable* _Nonnull out);

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
void libkrbn_system_preferences_monitor_terminate(libkrbn_system_preferences_monitor* _Nullable* _Nonnull out);

// ----------------------------------------
// libkrbn_device_monitor

typedef void libkrbn_device_monitor;
typedef void (*libkrbn_device_monitor_callback)(void* _Nullable refcon);
bool libkrbn_device_monitor_initialize(libkrbn_device_monitor* _Nullable* _Nonnull out,
                                       libkrbn_device_monitor_callback _Nullable callback,
                                       void* _Nullable refcon);
void libkrbn_device_monitor_terminate(libkrbn_device_monitor* _Nullable* _Nonnull out);

// ----------------------------------------
// libkrbn_log_monitor

typedef void libkrbn_log_monitor;
typedef void (*libkrbn_log_monitor_callback)(const char* _Nonnull line, void* _Nullable refcon);
bool libkrbn_log_monitor_initialize(libkrbn_log_monitor* _Nullable* _Nonnull out,
                                    libkrbn_log_monitor_callback _Nullable callback,
                                    void* _Nullable refcon);
void libkrbn_log_monitor_terminate(libkrbn_log_monitor* _Nullable* _Nonnull out);
size_t libkrbn_log_monitor_initial_lines_size(libkrbn_log_monitor* _Nonnull p);
const char* _Nullable libkrbn_log_monitor_initial_line(libkrbn_log_monitor* _Nonnull p, size_t index);
void libkrbn_log_monitor_start(libkrbn_log_monitor* _Nonnull p);

#ifdef __cplusplus
}
#endif
