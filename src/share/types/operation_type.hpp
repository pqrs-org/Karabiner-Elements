#pragma once

#include <nlohmann/json.hpp>

namespace krbn {
enum class operation_type : uint8_t {
  none,
  // session_monitor -> grabber
  console_user_id_changed,
  // console_user_server -> grabber
  connect_console_user_server,
  system_preferences_updated,
  frontmost_application_changed,
  input_source_changed,
  // event_viewer -> grabber
  temporarily_ignore_all_devices,
  get_manipulator_environment, // The grabber responds only if the client is code-signed with the same Team ID.
  // grabber -> event_viewer
  manipulator_environment,
  // multitouch_extension -> grabber
  connect_multitouch_extension,
  // any -> grabber
  set_app_icon,
  set_variables,
  // grabber -> any
  get_system_variables, // Return only the system.* entries from manipulator_environment.variables.
  system_variables,
  // grabber -> console_user_server
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
        {operation_type::get_system_variables, "get_system_variables"},
        {operation_type::system_variables, "system_variables"},
        {operation_type::shell_command_execution, "shell_command_execution"},
        {operation_type::select_input_source, "select_input_source"},
        {operation_type::software_function, "software_function"},
        {operation_type::end_, "end_"},
    });
} // namespace krbn
