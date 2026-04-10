import Foundation

struct SettingsWindowAlertContext: Codable {
  var servicesEnabled = true
  var coreDaemonsRunning = true
  var coreAgentsRunning = true
  var servicesWaitingSeconds = 0

  enum CodingKeys: String, CodingKey {
    case servicesEnabled = "services_enabled"
    case coreDaemonsRunning = "core_daemons_running"
    case coreAgentsRunning = "core_agents_running"
    case servicesWaitingSeconds = "services_waiting_seconds"
  }
}

struct SettingsWindowAlertState: Codable {
  var currentAlert: SettingsWindowAlert
  var alertContext: SettingsWindowAlertContext

  enum CodingKeys: String, CodingKey {
    case currentAlert = "current_alert"
    case alertContext = "alert_context"
  }
}
