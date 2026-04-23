import Foundation

enum SettingsWindowAlert: String, Codable {
  case none
  case doctor
  case settings
  case consoleUserServerNotConnected = "console_user_server_not_connected"
  case servicesDisabled = "services_disabled"
  case servicesNotRunning = "services_not_running"
  case inputMonitoringPermissions = "input_monitoring_permissions"
  case accessibility
  case virtualHidDeviceServiceClientNotConnected =
    "virtual_hid_device_service_client_not_connected"
  case driverVersionMismatched = "driver_version_mismatched"
  case driverNotActivated = "driver_not_activated"
  case driverNotConnected = "driver_not_connected"
}
