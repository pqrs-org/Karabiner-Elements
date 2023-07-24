import Foundation

final class UserSettings: ObservableObject {
  static let shared = UserSettings()

  @UserDefault("kStartAtLogin", defaultValue: false)
  var openAtLogin: Bool {
    willSet {
      objectWillChange.send()
    }
  }

  @UserDefault("kHideIconInDock", defaultValue: false)
  var hideIconInDock: Bool {
    willSet {
      objectWillChange.send()
    }
  }

  @UserDefault("kRelaunchAfterWakeUpFromSleep", defaultValue: true)
  var relaunchAfterWakeUpFromSleep: Bool {
    willSet {
      objectWillChange.send()
    }
  }

  @UserDefault("kRelaunchWait", defaultValue: 3)
  var relaunchWait: Int {
    willSet {
      objectWillChange.send()
    }
  }

  @UserDefault("kIgnoredAreaTop", defaultValue: 0)
  var ignoredAreaTop: Int {
    willSet {
      objectWillChange.send()
    }
  }

  @UserDefault("kIgnoredAreaBottom", defaultValue: 0)
  var ignoredAreaBottom: Int {
    willSet {
      objectWillChange.send()
    }
  }

  @UserDefault("kIgnoredAreaLeft", defaultValue: 0)
  var ignoredAreaLeft: Int {
    willSet {
      objectWillChange.send()
    }
  }

  @UserDefault("kIgnoredAreaRight", defaultValue: 0)
  var ignoredAreaRight: Int {
    willSet {
      objectWillChange.send()
    }
  }

  @UserDefault("kDelayBeforeTurnOff", defaultValue: 0)
  var delayBeforeTurnOff: Int {
    willSet {
      objectWillChange.send()
    }
  }

  @UserDefault("kDelayBeforeTurnOn", defaultValue: 0)
  var delayBeforeTurnOn: Int {
    willSet {
      objectWillChange.send()
    }
  }

  var targetArea: NSRect {
    let top = Double(ignoredAreaTop) / 100
    let bottom = Double(ignoredAreaBottom) / 100
    let left = Double(ignoredAreaLeft) / 100
    let right = Double(ignoredAreaRight) / 100

    return NSMakeRect(
      left,
      bottom,
      (1.0 - left - right),
      (1.0 - top - bottom))
  }
}
