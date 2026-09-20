import AppKit
import Combine
import Foundation

struct UIStatePayload: Decodable {
  struct MenuSettings: Decodable {
    var showIcon = false
    var showProfileName = false
    var showAdditionalMenuItems = false
    var showQuitConfirmationMenu = true
    var enableMultitouchExtension = false
  }

  struct NotificationWindowSettings: Decodable {
    enum Position: String, Decodable {
      case topLeft = "top_left"
      case topRight = "top_right"
      case bottomLeft = "bottom_left"
      case bottomRight = "bottom_right"
    }

    struct Colors: Decodable {
      struct Theme: Decodable {
        var backgroundColor = "system"
        var textColor = "system"
      }

      var light = Theme()
      var dark = Theme()
    }

    var enabled = false
    var position = Position.bottomRight
    var respectScreenVisibleFrame = true
    var showIcon = true
    var fontSize = 13
    var colors = Colors()
  }

  struct Profile: Decodable, Identifiable {
    let id: Int
    let name: String
    let selected: Bool
  }

  let uiLanguage: String
  let configurationLoaded: Bool
  let appIconNumber: Int
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

  @Published private(set) var uiLanguage = "auto"
  @Published private(set) var configurationLoaded = false
  @Published private(set) var appIconNumber = 0
  @Published private(set) var menuSettings = UIStatePayload.MenuSettings()
  @Published private(set) var notificationWindowSettings =
    UIStatePayload.NotificationWindowSettings()
  @Published private(set) var profiles: [UIStatePayload.Profile] = []
  @Published var notificationMessage = ""

  private var languageSubscriptions: Set<AnyCancellable> = []

  var menuVisible: Bool {
    configurationLoaded && (menuSettings.showIcon || menuSettings.showProfileName)
  }

  var selectedProfileName: String {
    profiles.first(where: { $0.selected })?.name ?? ""
  }

  func start() {
    if languageSubscriptions.isEmpty {
      AppLocalization.shared.$catalog
        .sink { [weak self] _ in
          // @Published sends before assignment; resolve using the updated catalog on the main actor.
          Task { @MainActor in self?.publishResolvedUILanguage() }
        }
        .store(in: &languageSubscriptions)
      NotificationCenter.default.publisher(for: NSLocale.currentLocaleDidChangeNotification)
        .sink { [weak self] _ in
          Task { @MainActor in self?.publishResolvedUILanguage() }
        }
        .store(in: &languageSubscriptions)
    }
    console_user_server_register_ui_state_callback(uiStateUpdated)
    console_user_server_register_notification_message_callback(notificationMessageUpdated)
  }

  func selectProfile(_ profile: UIStatePayload.Profile) {
    console_user_server_select_profile(profile.id)
  }

  fileprivate func apply(_ payload: UIStatePayload) {
    uiLanguage = payload.uiLanguage
    appIconNumber = payload.appIconNumber
    menuSettings = payload.menuSettings
    notificationWindowSettings = payload.notificationWindowSettings
    profiles = payload.profiles
    configurationLoaded = payload.configurationLoaded
    publishResolvedUILanguage()
    NotificationWindowManager.shared.updateWindowsVisibility()
  }

  private func publishResolvedUILanguage() {
    let language = AppLanguage.locale(for: uiLanguage).identifier
    console_user_server_set_resolved_ui_language(language)
  }
}
