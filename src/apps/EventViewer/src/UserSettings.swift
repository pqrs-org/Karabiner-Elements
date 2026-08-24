import SwiftUI

@MainActor
final class UserSettings: ObservableObject {
  static let shared = UserSettings()

  @AppStorage("kForceStayTop") var forceStayTop = false
  @AppStorage("kShowInAllSpaces") var showInAllSpaces = false
  @AppStorage("kShowUnknownEvents") var showUnknownEvents = true
}
