import SwiftUI

@MainActor
final class UserSettings: ObservableObject {
  static let shared = UserSettings()

  @AppStorage("kRelaunchAfterWakeUpFromSleep") var relaunchAfterWakeUpFromSleep = true
  @AppStorage("kRelaunchWait") var relaunchWait = 3
  @AppStorage("kIgnoredAreaTop") var ignoredAreaTop = 0
  @AppStorage("kIgnoredAreaBottom") var ignoredAreaBottom = 0
  @AppStorage("kIgnoredAreaLeft") var ignoredAreaLeft = 0
  @AppStorage("kIgnoredAreaRight") var ignoredAreaRight = 0
  @AppStorage("kDelayBeforeTurnOff") var delayBeforeTurnOff = 0
  @AppStorage("kDelayBeforeTurnOn") var delayBeforeTurnOn = 0
  @AppStorage("kPalmThreshold") var palmThreshold = 2.0

  var targetArea: NSRect {
    let top = Double(ignoredAreaTop) / 100
    let bottom = Double(ignoredAreaBottom) / 100
    let left = Double(ignoredAreaLeft) / 100
    let right = Double(ignoredAreaRight) / 100

    return NSRect(
      x: left,
      y: bottom,
      width: (1.0 - left - right),
      height: (1.0 - top - bottom))
  }
}
