#pragma once

#include <nlohmann/json.hpp>

namespace krbn {
enum class settings_window_alert : uint8_t {
  none,
  doctor,
  settings,
  services_not_running,
  input_monitoring_permissions,
  accessibility,
  virtual_hid_device_service_client_not_connected,
  driver_version_mismatched,
  driver_not_activated,
  driver_not_connected,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    settings_window_alert,
    {
        {settings_window_alert::none, "none"},
        {settings_window_alert::doctor, "doctor"},
        {settings_window_alert::settings, "settings"},
        {settings_window_alert::services_not_running, "services_not_running"},
        {settings_window_alert::input_monitoring_permissions, "input_monitoring_permissions"},
        {settings_window_alert::accessibility, "accessibility"},
        {settings_window_alert::virtual_hid_device_service_client_not_connected, "virtual_hid_device_service_client_not_connected"},
        {settings_window_alert::driver_version_mismatched, "driver_version_mismatched"},
        {settings_window_alert::driver_not_activated, "driver_not_activated"},
        {settings_window_alert::driver_not_connected, "driver_not_connected"},
    });
} // namespace krbn
