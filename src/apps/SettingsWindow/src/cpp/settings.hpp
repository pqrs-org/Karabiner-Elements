#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// The JSON data is valid only while the callback is being invoked.
typedef void (*krbn_core_configuration_updated_t)(const char* _Nonnull json,
                                                  size_t length);
typedef enum {
  krbn_core_configuration_load_state_loaded,
  krbn_core_configuration_load_state_permission_error,
  krbn_core_configuration_load_state_json_error,
  krbn_core_configuration_load_state_other_error,
} krbn_core_configuration_load_state;
typedef void (*krbn_core_configuration_load_state_changed_t)(krbn_core_configuration_load_state state);
typedef void (*krbn_log_messages_updated_t)(const char* _Nonnull json,
                                            size_t length);
typedef void (*krbn_core_service_daemon_client_connected_devices_received_t)(const char* _Nonnull json_string);
typedef void (*krbn_core_service_daemon_client_system_variables_received_t)(const char* _Nonnull json_string);
typedef void (*krbn_console_user_server_client_status_changed_t)(void);
typedef void (*krbn_console_user_server_client_settings_window_guidance_received_t)(const char* _Nonnull json_string);
typedef void (*krbn_components_manager_stopped_t)(uint64_t last_configuration_revision);
typedef void (*krbn_termination_completion_callback_t)(void);

void krbn_initialize(krbn_core_configuration_updated_t _Nonnull core_configuration_updated_callback,
                     krbn_core_configuration_load_state_changed_t _Nonnull core_configuration_load_state_changed_callback,
                     krbn_log_messages_updated_t _Nonnull log_messages_updated_callback,
                     krbn_core_service_daemon_client_connected_devices_received_t _Nonnull connected_devices_received_callback,
                     krbn_core_service_daemon_client_system_variables_received_t _Nonnull system_variables_received_callback,
                     krbn_console_user_server_client_status_changed_t _Nonnull console_user_server_client_status_changed_callback,
                     krbn_console_user_server_client_settings_window_guidance_received_t _Nonnull settings_window_guidance_received_callback,
                     krbn_components_manager_stopped_t _Nonnull components_manager_stopped_callback,
                     krbn_termination_completion_callback_t _Nonnull termination_completion_callback)
    __attribute__((swift_name(
        "krbn_initialize("
        "coreConfigurationUpdated:"
        "coreConfigurationLoadStateChanged:"
        "logMessagesUpdated:"
        "connectedDevicesReceived:"
        "systemVariablesReceived:"
        "consoleUserServerClientStatusChanged:"
        "settingsWindowGuidanceReceived:"
        "componentsManagerStopped:"
        "terminationCompleted:"
        ")")));
bool krbn_async_request_termination(void);
void krbn_finalize(void);

void krbn_load_custom_environment_variables(void);

// The JSON data is valid only while the callback is being invoked.
typedef void (*krbn_json_output_callback)(const char* _Nonnull json,
                                          size_t length);
typedef void (*krbn_json_output_callback_with_context)(const char* _Nonnull json,
                                                       size_t length,
                                                       void* _Nonnull context);

void krbn_get_user_configuration_directory(char* _Nonnull buffer,
                                           size_t length);
void krbn_get_user_complex_modifications_assets_directory(char* _Nonnull buffer,
                                                          size_t length);
void krbn_get_user_tmp_directory(char* _Nonnull buffer,
                                 size_t length);

void krbn_services_register_core_daemons(void);
void krbn_services_register_core_agents(void);
void krbn_services_bootout_and_disable_old_agents(void);
void krbn_services_restart_console_user_server_agent(void);
void krbn_services_unregister_all_agents(void);
typedef enum {
  krbn_service_enabled_state_unknown,
  krbn_service_enabled_state_disabled,
  krbn_service_enabled_state_enabled,
} krbn_service_enabled_state;
krbn_service_enabled_state krbn_services_daemons_enabled(void);
krbn_service_enabled_state krbn_services_agents_enabled(void);

void krbn_updater_check_for_updates_stable_only(void);
void krbn_updater_check_for_updates_with_beta_version(void);

void krbn_killall_settings(void);
void krbn_launch_uninstaller(void);

bool krbn_system_core_configuration_file_path_exists(void);

bool krbn_system_preferences_virtual_hid_keyboard_modifier_mappings_exists(void);

int krbn_get_app_icon_number(void);

void krbn_save_prettierrc(void);

//
// krbn_core_configuration
//

// Copies the command before returning. Completes exactly once, including when
// components are stopped. The callback may run on the dispatcher or caller thread.
void krbn_async_configuration_command(const char* _Nonnull json_string,
                                      krbn_json_output_callback_with_context _Nonnull output,
                                      void* _Nonnull context);

void krbn_core_configuration_get_new_complex_modifications_rule_json_string(char* _Nonnull buffer,
                                                                            size_t length);
void krbn_core_configuration_get_new_complex_modifications_rule_eval_js_string(char* _Nonnull buffer,
                                                                               size_t length);

bool krbn_eval_js_to_json_string(const char* _Nonnull code,
                                 char* _Nonnull buffer,
                                 size_t length,
                                 char* _Nonnull log_message_buffer,
                                 size_t log_message_buffer_length,
                                 char* _Nonnull error_message_buffer,
                                 size_t error_message_buffer_length)
    __attribute__((swift_name(
        "krbn_eval_js_to_json_string("
        "code:"
        "jsonBuffer:"
        "jsonBufferLength:"
        "logMessageBuffer:"
        "logMessageBufferLength:"
        "errorMessageBuffer:"
        "errorMessageBufferLength:"
        ")")));

void krbn_complex_modifications_assets_file_parse(const char* _Nonnull code,
                                                  const char* _Nonnull file_extension,
                                                  krbn_json_output_callback _Nonnull output);

//
// settings_core_service_daemon_client
//

void krbn_core_service_daemon_client_async_set_app_icon(int number);

//
// settings_console_user_server_client
//

bool krbn_console_user_server_client_connected(void);

#ifdef __cplusplus
}
#endif
