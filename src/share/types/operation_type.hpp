#pragma once

#include <nlohmann/json.hpp>

namespace krbn {
enum class operation_type : uint8_t {
  none,
  // any -> any
  handshake,
  shared_secret, // The response to the `handshake`
  // session_monitor -> core_service (daemon)
  console_user_id_changed,
  // core_service (agent) -> core_service (deamon) -> console_user_server
  core_service_bundle_permission_check_result,
  frontmost_application_changed,
  focused_ui_element_changed,
  // console_user_server -> core_service (deamon)
  system_preferences_updated,
  input_source_changed,
  // core_service (daemon) -> console_user_server
  get_user_core_configuration_file_path,
  core_service_daemon_state,
  check_for_updates_on_startup,
  register_menu_agent,
  unregister_menu_agent,
  register_multitouch_extension_agent,
  unregister_multitouch_extension_agent,
  register_notification_window_agent,
  unregister_notification_window_agent,
  shell_command_execution,
  send_user_command,
  select_input_source,
  software_function,
  // console_user_server -> core_service (daemon)
  user_core_configuration_file_path,
  // settings_window -> console_user_server
  get_settings_window_alert,
  // console_user_server -> settings_window
  settings_window_alert,
  // event_viewer -> console_user_server
  get_frontmost_application_history,
  // console_user_server -> event_viewer
  frontmost_application_history,
  // event_viewer -> core_service (daemon)
  temporarily_ignore_all_devices,
  get_manipulator_environment, // The core_service responds only if the client is code-signed with the same Team ID.
  // core_service (daemon) -> event_viewer
  manipulator_environment,
  // multitouch_extension -> core_service (daemon)
  connect_multitouch_extension,
  // any -> core_service (daemon)
  set_app_icon,
  set_variables,
  clear_user_variables,
  // core_service (daemon) -> any
  get_connected_devices,
  connected_devices,
  get_notification_message,
  notification_message,
  get_system_variables, // Return only the system.* entries from manipulator_environment.variables.
  system_variables,
  get_multitouch_extension_variables,
  multitouch_extension_variables,
  end_,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    operation_type,
    {
        {operation_type::none, nullptr},
        {operation_type::handshake, "handshake"},
        {operation_type::shared_secret, "shared_secret"},
        {operation_type::console_user_id_changed, "console_user_id_changed"},
        {operation_type::core_service_bundle_permission_check_result, "core_service_bundle_permission_check_result"},
        {operation_type::frontmost_application_changed, "frontmost_application_changed"},
        {operation_type::focused_ui_element_changed, "focused_ui_element_changed"},
        {operation_type::system_preferences_updated, "system_preferences_updated"},
        {operation_type::input_source_changed, "input_source_changed"},
        {operation_type::get_user_core_configuration_file_path, "get_user_core_configuration_file_path"},
        {operation_type::core_service_daemon_state, "core_service_daemon_state"},
        {operation_type::user_core_configuration_file_path, "user_core_configuration_file_path"},
        {operation_type::shell_command_execution, "shell_command_execution"},
        {operation_type::send_user_command, "send_user_command,"},
        {operation_type::select_input_source, "select_input_source"},
        {operation_type::software_function, "software_function"},
        {operation_type::get_settings_window_alert, "get_settings_window_alert"},
        {operation_type::settings_window_alert, "settings_window_alert"},
        {operation_type::get_frontmost_application_history, "get_frontmost_application_history"},
        {operation_type::check_for_updates_on_startup, "check_for_updates_on_startup"},
        {operation_type::register_menu_agent, "register_menu_agent"},
        {operation_type::unregister_menu_agent, "unregister_menu_agent"},
        {operation_type::register_multitouch_extension_agent, "register_multitouch_extension_agent"},
        {operation_type::unregister_multitouch_extension_agent, "unregister_multitouch_extension_agent"},
        {operation_type::register_notification_window_agent, "register_notification_window_agent"},
        {operation_type::unregister_notification_window_agent, "unregister_notification_window_agent"},
        {operation_type::frontmost_application_history, "frontmost_application_history"},
        {operation_type::temporarily_ignore_all_devices, "temporarily_ignore_all_devices"},
        {operation_type::get_manipulator_environment, "get_manipulator_environment"},
        {operation_type::manipulator_environment, "manipulator_environment"},
        {operation_type::connect_multitouch_extension, "connect_multitouch_extension"},
        {operation_type::set_app_icon, "set_app_icon"},
        {operation_type::set_variables, "set_variables"},
        {operation_type::clear_user_variables, "clear_user_variables"},
        {operation_type::get_connected_devices, "get_connected_devices"},
        {operation_type::connected_devices, "connected_devices"},
        {operation_type::get_notification_message, "get_notification_message"},
        {operation_type::notification_message, "notification_message"},
        {operation_type::get_system_variables, "get_system_variables"},
        {operation_type::system_variables, "system_variables"},
        {operation_type::get_multitouch_extension_variables, "get_multitouch_extension_variables"},
        {operation_type::multitouch_extension_variables, "multitouch_extension_variables"},
        {operation_type::end_, "end_"},
    });
} // namespace krbn
