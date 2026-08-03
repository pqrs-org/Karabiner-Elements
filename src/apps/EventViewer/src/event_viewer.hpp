#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*krbn_core_service_connection_changed_callback)(bool connected);
typedef void (*krbn_json_received_callback)(const char* _Nonnull json_string);
typedef void (*krbn_hid_value_monitor_stopped_callback)(void);
typedef void (*krbn_hid_value_arrived_callback)(uint64_t device_id,
                                                int32_t usage_page,
                                                int32_t usage,
                                                int64_t integer_value,
                                                const char* _Nullable momentary_switch_event_json_string,
                                                const char* _Nullable modifier_flag_name);

void krbn_initialize(krbn_core_service_connection_changed_callback _Nonnull core_service_connection_changed_callback,
                     krbn_json_received_callback _Nonnull manipulator_environment_received_callback,
                     krbn_json_received_callback _Nonnull connected_devices_received_callback,
                     krbn_json_received_callback _Nonnull frontmost_application_history_received_callback,
                     krbn_hid_value_monitor_stopped_callback _Nonnull hid_value_monitor_stopped_callback,
                     krbn_hid_value_arrived_callback _Nonnull hid_value_arrived_callback);
void krbn_terminate(void);

void krbn_core_service_async_get_manipulator_environment(void);
void krbn_core_service_async_get_connected_devices(void);
void krbn_core_service_async_temporarily_ignore_all_devices(bool value);
void krbn_core_service_async_clear_user_variables(void);

void krbn_console_user_server_async_get_frontmost_application_history(void);

#ifdef __cplusplus
}
#endif
