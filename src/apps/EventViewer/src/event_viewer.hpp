#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*krbn_core_service_connection_changed_callback)(bool connected);
typedef void (*krbn_json_received_callback)(const char* _Nonnull json_string);
typedef void (*krbn_hid_value_monitor_stopped_callback)(void);
typedef void (*krbn_termination_completion_callback)(void);
typedef void (*krbn_hid_value_arrived_callback)(uint64_t device_id,
                                                int32_t usage_page,
                                                int32_t usage,
                                                int64_t integer_value,
                                                const char* _Nullable momentary_switch_event_json_string,
                                                const char* _Nullable modifier_flag_name);
typedef void (*krbn_hid_input_report_arrived_callback)(uint64_t device_id,
                                                       uint32_t report_id,
                                                       const uint8_t* _Nullable bytes,
                                                       size_t length);
typedef void (*krbn_hid_device_open_state_changed_callback)(uint64_t device_id,
                                                            bool opened);

void krbn_initialize(krbn_core_service_connection_changed_callback _Nonnull core_service_connection_changed_callback,
                     krbn_json_received_callback _Nonnull manipulator_environment_received_callback,
                     krbn_json_received_callback _Nonnull connected_devices_received_callback,
                     krbn_json_received_callback _Nonnull frontmost_application_history_received_callback,
                     krbn_hid_value_monitor_stopped_callback _Nonnull hid_value_monitor_stopped_callback,
                     krbn_hid_value_arrived_callback _Nonnull hid_value_arrived_callback,
                     krbn_hid_input_report_arrived_callback _Nonnull hid_input_report_arrived_callback,
                     krbn_hid_device_open_state_changed_callback _Nonnull hid_device_open_state_changed_callback,
                     krbn_termination_completion_callback _Nonnull termination_completion_callback)
    __attribute__((swift_name(
        "krbn_initialize("
        "coreServiceConnectionChanged:"
        "manipulatorEnvironmentReceived:"
        "connectedDevicesReceived:"
        "frontmostApplicationHistoryReceived:"
        "hidValueMonitorStopped:"
        "hidValueArrived:"
        "hidInputReportArrived:"
        "hidDeviceOpenStateChanged:"
        "terminationCompleted:"
        ")")));
bool krbn_async_request_termination(void);
void krbn_finalize(void);

void krbn_core_service_async_get_manipulator_environment(void);
void krbn_core_service_async_clear_user_variables(void);
void krbn_set_hid_capture_target(bool active, uint64_t device_id);
void krbn_set_hid_input_report_capture_device(uint64_t device_id);

void krbn_console_user_server_async_get_frontmost_application_history(void);

#ifdef __cplusplus
}
#endif
