import SwiftUI

@MainActor
final class UserSettings: ObservableObject {
  static let shared = UserSettings()

  @AppStorage("kForceStayTop") var forceStayTop = false
  @AppStorage("kShowInAllSpaces") var showInAllSpaces = false
  @Published var captureMouseWheels = false {
    didSet {
      krbn_set_hid_capture_mouse_wheels(captureMouseWheels)
    }
  }
  @Published var captureUnknownEvents = true

  private init() {
    krbn_set_hid_capture_mouse_wheels(captureMouseWheels)
  }
}
