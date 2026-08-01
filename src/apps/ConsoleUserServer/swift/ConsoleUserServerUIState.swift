import AppKit
import Foundation

struct UIStatePayload: Decodable {
  struct MenuSettings: Decodable {
    var showIcon = false
    var showProfileName = false
    var showAdditionalMenuItems = false
    var enableMultitouchExtension = false
    var askForConfirmationBeforeQuitting = false
  }

  struct NotificationWindowSettings: Decodable {
    var enabled = false
  }

  struct Profile: Decodable, Identifiable {
    let id: Int
    let name: String
    let selected: Bool
  }

  let menuSettings: MenuSettings
  let notificationWindowSettings: NotificationWindowSettings
  let profiles: [Profile]
}

private func uiStateUpdated(_ value: UnsafePointer<CChar>?) {
  guard let value else { return }
  let data = Data(String(cString: value).utf8)
  guard let payload = try? JSONDecoder().decode(UIStatePayload.self, from: data) else { return }

  Task { @MainActor in
    ConsoleUserServerUIState.shared.apply(payload)
  }
}

private func notificationMessageUpdated(_ value: UnsafePointer<CChar>?) {
  guard let value else { return }
  let message = String(cString: value)
  Task { @MainActor in
    ConsoleUserServerUIState.shared.notificationMessage = message
    NotificationWindowManager.shared.updateWindowsVisibility()
  }
}

@MainActor
final class ConsoleUserServerUIState: ObservableObject {
  static let shared = ConsoleUserServerUIState()

  @Published private(set) var menuSettings = UIStatePayload.MenuSettings()
  @Published private(set) var notificationWindowSettings =
    UIStatePayload.NotificationWindowSettings()
  @Published private(set) var profiles: [UIStatePayload.Profile] = []
  @Published var notificationMessage = ""

  var menuVisible: Bool { menuSettings.showIcon || menuSettings.showProfileName }

  var selectedProfileName: String {
    profiles.first(where: { $0.selected })?.name ?? ""
  }

  func start() {
    console_user_server_register_ui_state_callback(uiStateUpdated)
    console_user_server_register_notification_message_callback(notificationMessageUpdated)
  }

  func selectProfile(_ profile: UIStatePayload.Profile) {
    console_user_server_select_profile(profile.id)
  }

  fileprivate func apply(_ payload: UIStatePayload) {
    menuSettings = payload.menuSettings
    notificationWindowSettings = payload.notificationWindowSettings
    profiles = payload.profiles
    NotificationWindowManager.shared.updateWindowsVisibility()
  }
}
