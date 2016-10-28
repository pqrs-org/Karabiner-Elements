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

uint32_t libkrbn_get_keyboard_type_ansi(void);
uint32_t libkrbn_get_keyboard_type_iso(void);
uint32_t libkrbn_get_keyboard_type_jis(void);

typedef void libkrbn_configuration_monitor;
typedef void (*libkrbn_configuration_monitor_callback)(const char* _Nonnull current_profile_json, void* _Nullable refcon);
bool libkrbn_configuration_monitor_initialize(libkrbn_configuration_monitor* _Nullable* _Nonnull out,
                                              libkrbn_configuration_monitor_callback _Nullable callback,
                                              void* _Nullable refcon);
void libkrbn_configuration_monitor_terminate(libkrbn_configuration_monitor* _Nullable* _Nonnull out);

typedef void libkrbn_device_monitor;
typedef void (*libkrbn_device_monitor_callback)(void* _Nullable refcon);
bool libkrbn_device_monitor_initialize(libkrbn_device_monitor* _Nullable* _Nonnull out,
                                       libkrbn_device_monitor_callback _Nullable callback,
                                       void* _Nullable refcon);
void libkrbn_device_monitor_terminate(libkrbn_device_monitor* _Nullable* _Nonnull out);

typedef void libkrbn_log_monitor;
typedef void (*libkrbn_log_monitor_callback)(const char* _Nonnull line, void* _Nullable refcon);
bool libkrbn_log_monitor_initialize(libkrbn_log_monitor* _Nullable* _Nonnull out,
                                    libkrbn_log_monitor_callback _Nullable callback,
                                    void* _Nullable refcon);
void libkrbn_log_monitor_terminate(libkrbn_log_monitor* _Nullable* _Nonnull out);
size_t libkrbn_log_monitor_initial_lines_size(libkrbn_log_monitor* _Nonnull p);
const char* _Nullable libkrbn_log_monitor_initial_line(libkrbn_log_monitor* _Nonnull p, size_t index);
void libkrbn_log_monitor_start(libkrbn_log_monitor* _Nonnull p);

bool libkrbn_get_hid_system_key(uint8_t* _Nonnull key, const char* _Nonnull key_name);
bool libkrbn_get_hid_system_aux_control_button(uint8_t* _Nonnull button, const char* _Nonnull key_name);

bool libkrbn_save_beautified_json_string(const char* _Nonnull file_path, const char* _Nonnull json_string);

#ifdef __cplusplus
}
#endif
