import Foundation

struct SettingsWindowCoreServicePermissionCheckResult: Codable {
  var iohidListenEventAllowed = false
  var accessibilityProcessTrusted = false

  enum CodingKeys: String, CodingKey {
    case iohidListenEventAllowed = "iohid_listen_event_allowed"
    case accessibilityProcessTrusted = "accessibility_process_trusted"
  }
}

struct SettingsWindowCoreServiceState: Codable {
  var virtualHidDeviceServiceClientConnected: Bool?
  var driverActivated: Bool?
  var driverConnected: Bool?
  var driverVersionMismatched: Bool?
  var virtualHidKeyboardReady: Bool?
  var currentProcessPermissionCheckResult: SettingsWindowCoreServicePermissionCheckResult?
  var bundlePermissionCheckResult: SettingsWindowCoreServicePermissionCheckResult?
  var karabinerJsonParseErrorMessage = ""
  var virtualHidKeyboardTypeNotSet: Bool?

  var inputMonitoringGranted: Bool? {
    bundlePermissionCheckResult?.iohidListenEventAllowed
  }

  var accessibilityProcessTrusted: Bool? {
    bundlePermissionCheckResult?.accessibilityProcessTrusted
  }

  enum CodingKeys: String, CodingKey {
    case virtualHidDeviceServiceClientConnected = "virtual_hid_device_service_client_connected"
    case driverActivated = "driver_activated"
    case driverConnected = "driver_connected"
    case driverVersionMismatched = "driver_version_mismatched"
    case virtualHidKeyboardReady = "virtual_hid_keyboard_ready"
    case currentProcessPermissionCheckResult = "current_process_permission_check_result"
    case bundlePermissionCheckResult = "bundle_permission_check_result"
    case karabinerJsonParseErrorMessage = "karabiner_json_parse_error_message"
    case virtualHidKeyboardTypeNotSet = "virtual_hid_keyboard_type_not_set"
  }
}

struct SettingsWindowGuidanceContext: Codable {
  var servicesEnabled = true
  var coreDaemonsEnabled = true
  var coreAgentsEnabled = true
  var coreDaemonsRunning = true
  var coreAgentsRunning = true
  var servicesWaitingSeconds = 0

  enum CodingKeys: String, CodingKey {
    case servicesEnabled = "services_enabled"
    case coreDaemonsEnabled = "core_daemons_enabled"
    case coreAgentsEnabled = "core_agents_enabled"
    case coreDaemonsRunning = "core_daemons_running"
    case coreAgentsRunning = "core_agents_running"
    case servicesWaitingSeconds = "services_waiting_seconds"
  }
}

struct SettingsWindowGuidanceState: Codable {
  var currentSetup: SettingsWindowGuidanceSetup
  var currentAlert: SettingsWindowGuidanceAlert
  var guidanceContext: SettingsWindowGuidanceContext
  var coreServiceDaemonState = SettingsWindowCoreServiceState()

  enum CodingKeys: String, CodingKey {
    case currentSetup = "current_setup"
    case currentAlert = "current_alert"
    case guidanceContext = "guidance_context"
    case coreServiceDaemonState = "core_service_daemon_state"
  }
}
