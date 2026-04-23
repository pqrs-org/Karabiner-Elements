import Foundation

enum SettingsWindowAlert: String, Codable {
  case none
  case doctor
  case settings
  case consoleUserServerNotConnected = "console_user_server_not_connected"
  case servicesNotRunning = "services_not_running"
  case virtualHidDeviceServiceClientNotConnected =
    "virtual_hid_device_service_client_not_connected"
  case driverVersionMismatched = "driver_version_mismatched"
  case driverNotConnected = "driver_not_connected"
}

enum SettingsWindowSetup: String, Codable {
  case none
  case services
  case accessibility
  case inputMonitoring = "input_monitoring"
  case driverExtension = "driver_extension"
}
