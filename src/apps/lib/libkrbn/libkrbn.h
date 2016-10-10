#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* libkrbn_get_distributed_notification_observed_object(void);
const char* libkrbn_get_distributed_notification_grabber_is_launched(void);
const char* libkrbn_get_configuration_core_file_path(void);

typedef void libkrbn_configuration_monitor;
typedef void (*libkrbn_configuration_monitor_callback)(const char* current_profile_json, void* refcon);
bool libkrbn_configuration_monitor_initialize(libkrbn_configuration_monitor** out, libkrbn_configuration_monitor_callback callback, void* refcon);
void libkrbn_configuration_monitor_terminate(libkrbn_configuration_monitor** out);

typedef void libkrbn_log_monitor;
typedef void (*libkrbn_log_monitor_callback)(const char* line, void* refcon);
bool libkrbn_log_monitor_initialize(libkrbn_log_monitor** out, libkrbn_log_monitor_callback callback, void* refcon);
void libkrbn_log_monitor_terminate(libkrbn_log_monitor** out);
size_t libkrbn_log_monitor_initial_lines_size(libkrbn_log_monitor* p);
const char* libkrbn_log_monitor_initial_line(libkrbn_log_monitor* p, size_t index);
void libkrbn_log_monitor_start(libkrbn_log_monitor* p);

#ifdef __cplusplus
}
#endif
