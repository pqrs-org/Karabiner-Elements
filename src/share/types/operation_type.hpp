#pragma once

#include <nlohmann/json.hpp>

namespace krbn {
enum class operation_type : uint8_t {
  none,
  // session_monitor -> core_service
  console_user_id_changed,
  // console_user_server -> core_service
  connect_console_user_server,
  system_preferences_updated,
  frontmost_application_changed,
  input_source_changed,
  // event_viewer -> core_service
  temporarily_ignore_all_devices,
  get_manipulator_environment, // The core_service responds only if the client is code-signed with the same Team ID.
  // core_service -> event_viewer
  manipulator_environment,
  // multitouch_extension -> core_service
  connect_multitouch_extension,
  // any -> core_service
  set_app_icon,
  set_variables,
  // core_service -> any
  get_connected_devices,
  connected_devices,
  get_notification_message,
  notification_message,
  get_system_variables, // Return only the system.* entries from manipulator_environment.variables.
  system_variables,
  get_multitouch_extension_variables,
  multitouch_extension_variables,
  // core_service -> console_user_server
  shell_command_execution,
  select_input_source,
  software_function,
  end_,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    operation_type,
    {
        {operation_type::none, nullptr},
        {operation_type::console_user_id_changed, "console_user_id_changed"},
        {operation_type::connect_console_user_server, "connect_console_user_server"},
        {operation_type::system_preferences_updated, "system_preferences_updated"},
        {operation_type::frontmost_application_changed, "frontmost_application_changed"},
        {operation_type::input_source_changed, "input_source_changed"},
        {operation_type::temporarily_ignore_all_devices, "temporarily_ignore_all_devices"},
        {operation_type::get_manipulator_environment, "get_manipulator_environment"},
        {operation_type::manipulator_environment, "manipulator_environment"},
        {operation_type::connect_multitouch_extension, "connect_multitouch_extension"},
        {operation_type::set_app_icon, "set_app_icon"},
        {operation_type::set_variables, "set_variables"},
        {operation_type::get_connected_devices, "get_connected_devices"},
        {operation_type::connected_devices, "connected_devices"},
        {operation_type::get_notification_message, "get_notification_message"},
        {operation_type::notification_message, "notification_message"},
        {operation_type::get_system_variables, "get_system_variables"},
        {operation_type::system_variables, "system_variables"},
        {operation_type::get_multitouch_extension_variables, "get_multitouch_extension_variables"},
        {operation_type::multitouch_extension_variables, "multitouch_extension_variables"},
        {operation_type::shell_command_execution, "shell_command_execution"},
        {operation_type::select_input_source, "select_input_source"},
        {operation_type::software_function, "software_function"},
        {operation_type::end_, "end_"},
    });
} // namespace krbn
