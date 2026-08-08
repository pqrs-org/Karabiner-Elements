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
typedef void (*krbn_log_messages_updated_t)(const char* _Nonnull json,
                                            size_t length);
typedef void (*krbn_core_service_daemon_client_connected_devices_received_t)(const char* _Nonnull json_string);
typedef void (*krbn_core_service_daemon_client_system_variables_received_t)(const char* _Nonnull json_string);
typedef void (*krbn_console_user_server_client_status_changed_t)(void);
typedef void (*krbn_console_user_server_client_settings_window_guidance_received_t)(const char* _Nonnull json_string);

void krbn_initialize(krbn_core_configuration_updated_t _Nonnull core_configuration_updated_callback,
                     krbn_log_messages_updated_t _Nonnull log_messages_updated_callback,
                     krbn_core_service_daemon_client_connected_devices_received_t _Nonnull connected_devices_received_callback,
                     krbn_core_service_daemon_client_system_variables_received_t _Nonnull system_variables_received_callback,
                     krbn_console_user_server_client_status_changed_t _Nonnull console_user_server_client_status_changed_callback,
                     krbn_console_user_server_client_settings_window_guidance_received_t _Nonnull settings_window_guidance_received_callback)
    __attribute__((swift_name(
        "krbn_initialize("
        "coreConfigurationUpdated:"
        "logMessagesUpdated:"
        "connectedDevicesReceived:"
        "systemVariablesReceived:"
        "consoleUserServerClientStatusChanged:"
        "settingsWindowGuidanceReceived:"
        ")")));
void krbn_terminate(void);

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
void krbn_services_bootout_old_agents(void);
void krbn_services_restart_console_user_server_agent(void);
void krbn_services_unregister_all_agents(void);
bool krbn_services_daemons_enabled(void);
bool krbn_services_agents_enabled(void);

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

bool krbn_core_configuration_save(char* _Nonnull error_message_buffer,
                                  size_t error_message_buffer_length);
void krbn_core_configuration_get_settings_configuration_snapshot_json(krbn_json_output_callback_with_context _Nonnull output,
                                                                      void* _Nonnull context);

// global_configuration

void krbn_core_configuration_set_global_configuration_check_for_updates(bool value);
void krbn_core_configuration_set_global_configuration_show_in_menu_bar(bool value);
void krbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(bool value);
void krbn_core_configuration_set_global_configuration_show_additional_menu_items(bool value);
void krbn_core_configuration_set_global_configuration_enable_notification_window(bool value);
void krbn_core_configuration_set_global_configuration_unsafe_ui(bool value);
void krbn_core_configuration_set_global_configuration_filter_useless_events_from_specific_devices(bool value);
void krbn_core_configuration_set_global_configuration_reorder_same_timestamp_input_events_to_prioritize_modifiers(bool value);
void krbn_core_configuration_set_global_configuration_enable_cgeventtap_fallback(bool value);

// machine_specific

void krbn_core_configuration_set_machine_specific_enable_multitouch_extension(bool value);
void krbn_core_configuration_set_machine_specific_external_editor_path(const char* _Nonnull value);

// profiles

void krbn_core_configuration_set_profile_name(size_t index,
                                              const char* _Nonnull value);
void krbn_core_configuration_select_profile(size_t index);
void krbn_core_configuration_push_back_profile(void);
void krbn_core_configuration_duplicate_profile(size_t source_index);
void krbn_core_configuration_move_profile(size_t source_index, size_t destination_index);
void krbn_core_configuration_erase_profile(size_t index);

// profile::parameters

void krbn_core_configuration_set_selected_profile_parameters_delay_milliseconds_before_open_device(int value);

// profile::simple_modifications

void krbn_core_configuration_replace_selected_profile_simple_modification(size_t index,
                                                                          const char* _Nonnull from_json_string,
                                                                          const char* _Nonnull to_json_string,
                                                                          const char* _Nullable device_identifiers_json)
    __attribute__((swift_name(
        "krbn_core_configuration_replace_selected_profile_simple_modification("
        "index:"
        "fromJSON:"
        "toJSON:"
        "deviceIdentifiersJSON:"
        ")")));
void krbn_core_configuration_push_back_selected_profile_simple_modification(const char* _Nullable device_identifiers_json);
void krbn_core_configuration_erase_selected_profile_simple_modification(size_t index,
                                                                        const char* _Nullable device_identifiers_json);

// profile::fn_function_keys

void krbn_core_configuration_replace_selected_profile_fn_function_key(const char* _Nonnull from_json_string,
                                                                      const char* _Nonnull to_json_string,
                                                                      const char* _Nullable device_identifiers_json)
    __attribute__((swift_name(
        "krbn_core_configuration_replace_selected_profile_fn_function_key("
        "fromJSON:"
        "toJSON:"
        "deviceIdentifiersJSON:"
        ")")));

// profile:complex_modifications

void krbn_core_configuration_set_selected_profile_complex_modifications_rule_enabled(size_t index, bool value);

typedef enum {
  krbn_complex_modifications_rule_code_type_json,
  krbn_complex_modifications_rule_code_type_javascript,
} krbn_complex_modifications_rule_code_type;
void krbn_core_configuration_replace_selected_profile_complex_modifications_rule(size_t index,
                                                                                 const char* _Nonnull code_string,
                                                                                 krbn_complex_modifications_rule_code_type code_type,
                                                                                 char* _Nonnull error_message_buffer,
                                                                                 size_t error_message_buffer_length)
    __attribute__((swift_name(
        "krbn_core_configuration_replace_selected_profile_complex_modifications_rule("
        "index:"
        "code:"
        "codeType:"
        "errorMessageBuffer:"
        "errorMessageBufferLength:"
        ")")));
void krbn_core_configuration_push_front_selected_profile_complex_modifications_rule(const char* _Nonnull code_string,
                                                                                    krbn_complex_modifications_rule_code_type code_type,
                                                                                    char* _Nonnull error_message_buffer,
                                                                                    size_t error_message_buffer_length)
    __attribute__((swift_name(
        "krbn_core_configuration_push_front_selected_profile_complex_modifications_rule("
        "code:"
        "codeType:"
        "errorMessageBuffer:"
        "errorMessageBufferLength:"
        ")")));
void krbn_core_configuration_erase_selected_profile_complex_modifications_rule(size_t index);
void krbn_core_configuration_move_selected_profile_complex_modifications_rule(size_t source_index, size_t destination_index);

void krbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_simultaneous_threshold_milliseconds(int value);

void krbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_if_alone_timeout_milliseconds(int value);

void krbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_if_held_down_threshold_milliseconds(int value);

void krbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_delayed_action_delay_milliseconds(int value);

void krbn_core_configuration_set_selected_profile_complex_modifications_parameter_mouse_motion_to_scroll_speed(int value);

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

// profile::virtual_hid_device

void krbn_core_configuration_set_selected_profile_virtual_hid_keyboard_keyboard_type_v2(const char* _Nonnull value);

void krbn_core_configuration_set_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(int value);

void krbn_core_configuration_set_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(bool value);

// profile::devices

void krbn_core_configuration_set_selected_profile_device_ignore(const char* _Nullable device_identifiers_json,
                                                                bool value);
void krbn_core_configuration_set_selected_profile_device_manipulate_caps_lock_led(const char* _Nullable device_identifiers_json,
                                                                                  bool value);
void krbn_core_configuration_set_selected_profile_device_ignore_vendor_events(const char* _Nullable device_identifiers_json,
                                                                              bool value);
void krbn_core_configuration_set_selected_profile_device_treat_as_built_in_keyboard(const char* _Nullable device_identifiers_json,
                                                                                    bool value);
void krbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(const char* _Nullable device_identifiers_json,
                                                                                             bool value);
void krbn_core_configuration_set_selected_profile_device_pointing_motion_xy_multiplier(const char* _Nullable device_identifiers_json,
                                                                                       double value);
void krbn_core_configuration_set_selected_profile_device_pointing_motion_wheels_multiplier(const char* _Nullable device_identifiers_json,
                                                                                           double value);
void krbn_core_configuration_set_selected_profile_device_mouse_flip_x(const char* _Nullable device_identifiers_json,
                                                                      bool value);
void krbn_core_configuration_set_selected_profile_device_mouse_flip_y(const char* _Nullable device_identifiers_json,
                                                                      bool value);
void krbn_core_configuration_set_selected_profile_device_mouse_flip_vertical_wheel(const char* _Nullable device_identifiers_json,
                                                                                   bool value);
void krbn_core_configuration_set_selected_profile_device_mouse_flip_horizontal_wheel(const char* _Nullable device_identifiers_json,
                                                                                     bool value);

void krbn_core_configuration_set_selected_profile_device_mouse_discard_x(const char* _Nullable device_identifiers_json,
                                                                         bool value);
void krbn_core_configuration_set_selected_profile_device_mouse_discard_y(const char* _Nullable device_identifiers_json,
                                                                         bool value);
void krbn_core_configuration_set_selected_profile_device_mouse_discard_vertical_wheel(const char* _Nullable device_identifiers_json,
                                                                                      bool value);
void krbn_core_configuration_set_selected_profile_device_mouse_discard_horizontal_wheel(const char* _Nullable device_identifiers_json,
                                                                                        bool value);

void krbn_core_configuration_set_selected_profile_device_mouse_swap_xy(const char* _Nullable device_identifiers_json,
                                                                       bool value);
void krbn_core_configuration_set_selected_profile_device_mouse_swap_wheels(const char* _Nullable device_identifiers_json,
                                                                           bool value);
void krbn_core_configuration_set_selected_profile_device_game_pad_swap_sticks(const char* _Nullable device_identifiers_json,
                                                                              bool value);

size_t krbn_core_configuration_get_selected_profile_not_connected_configured_devices_count(const char* _Nonnull connected_devices_json);
void krbn_core_configuration_erase_selected_profile_not_connected_configured_devices(const char* _Nonnull connected_devices_json);

// game_pad_xy_stick_deadzone

void krbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_deadzone(const char* _Nullable device_identifiers_json,
                                                                                    double value);

// game_pad_xy_stick_delta_magnitude_detection_threshold

void krbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_delta_magnitude_detection_threshold(const char* _Nullable device_identifiers_json,
                                                                                                               double value);

// game_pad_xy_stick_continued_movement_absolute_magnitude_threshold

void krbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(const char* _Nullable device_identifiers_json,
                                                                                                                           double value);

// game_pad_xy_stick_continued_movement_interval_milliseconds

void krbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(const char* _Nullable device_identifiers_json,
                                                                                                                    int value);

// game_pad_wheels_stick_deadzone

void krbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_deadzone(const char* _Nullable device_identifiers_json,
                                                                                        double value);

// game_pad_wheels_stick_delta_magnitude_detection_threshold

void krbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_delta_magnitude_detection_threshold(const char* _Nullable device_identifiers_json,
                                                                                                                   double value);

// game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold

void krbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(const char* _Nullable device_identifiers_json,
                                                                                                                               double value);

// game_pad_wheels_stick_continued_movement_interval_milliseconds

void krbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(const char* _Nullable device_identifiers_json,
                                                                                                                        int value);

// game_pad_stick_x_formula

bool krbn_core_configuration_set_selected_profile_device_game_pad_stick_x_formula(const char* _Nullable device_identifiers_json,
                                                                                  const char* _Nonnull value);
void krbn_core_configuration_reset_selected_profile_device_game_pad_stick_x_formula(const char* _Nullable device_identifiers_json);

// game_pad_stick_y_formula

bool krbn_core_configuration_set_selected_profile_device_game_pad_stick_y_formula(const char* _Nullable device_identifiers_json,
                                                                                  const char* _Nonnull value);
void krbn_core_configuration_reset_selected_profile_device_game_pad_stick_y_formula(const char* _Nullable device_identifiers_json);

// game_pad_stick_vertical_wheel_formula

bool krbn_core_configuration_set_selected_profile_device_game_pad_stick_vertical_wheel_formula(const char* _Nullable device_identifiers_json,
                                                                                               const char* _Nonnull value);
void krbn_core_configuration_reset_selected_profile_device_game_pad_stick_vertical_wheel_formula(const char* _Nullable device_identifiers_json);

// game_pad_stick_horizontal_wheel_formula

bool krbn_core_configuration_set_selected_profile_device_game_pad_stick_horizontal_wheel_formula(const char* _Nullable device_identifiers_json,
                                                                                                 const char* _Nonnull value);
void krbn_core_configuration_reset_selected_profile_device_game_pad_stick_horizontal_wheel_formula(const char* _Nullable device_identifiers_json);

//
// settings_complex_modifications_assets_manager
//

void krbn_complex_modifications_assets_manager_reload(krbn_json_output_callback _Nonnull output);

void krbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(size_t file_index,
                                                                                               size_t index);
void krbn_complex_modifications_assets_manager_erase_file(size_t index);

//
// settings_core_service_daemon_client
//

void krbn_core_service_daemon_client_async_get_connected_devices(void);

void krbn_core_service_daemon_client_async_get_system_variables(void);

void krbn_core_service_daemon_client_async_set_app_icon(int number);

//
// settings_console_user_server_client
//

typedef enum {
  krbn_console_user_server_client_status_none,
  krbn_console_user_server_client_status_connected,
  krbn_console_user_server_client_status_connect_failed,
  krbn_console_user_server_client_status_closed,
} krbn_console_user_server_client_status;

krbn_console_user_server_client_status krbn_console_user_server_client_get_status(void);

void krbn_console_user_server_client_async_get_settings_window_guidance(void);

#ifdef __cplusplus
}
#endif
