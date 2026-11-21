import SwiftUI

@MainActor
final class UserSettings: ObservableObject {
  static let shared = UserSettings()

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
