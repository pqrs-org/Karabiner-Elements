import Foundation

final class UserSettings: ObservableObject {
  static let shared = UserSettings()
  static let windowSettingChanged = Notification.Name("windowSettingChanged")

  @UserDefault("kForceStayTop", defaultValue: false)
  var forceStayTop: Bool {
    willSet {
      objectWillChange.send()
    }
    didSet {
      NotificationCenter.default.post(
        name: UserSettings.windowSettingChanged,
        object: nil
      )
    }
  }

  @UserDefault("kShowInAllSpaces", defaultValue: false)
  var showInAllSpaces: Bool {
    willSet {
      objectWillChange.send()
    }
    didSet {
      NotificationCenter.default.post(
        name: UserSettings.windowSettingChanged,
        object: nil
      )
    }
  }

  @UserDefault("quitUsingKeyboardShortcut", defaultValue: false)
  var quitUsingKeyboardShortcut: Bool {
    willSet {
      objectWillChange.send()
    }
  }
}
