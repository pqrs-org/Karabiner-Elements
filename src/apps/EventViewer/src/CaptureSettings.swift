import SwiftUI

@MainActor
final class CaptureSettings: ObservableObject {
  static let shared = CaptureSettings()

  @Published var captureMouseWheels = false {
    didSet {
      krbn_set_hid_capture_mouse_wheels(captureMouseWheels)
    }
  }
  @Published var captureUnknownEvents = true
  @Published var captureMacOSKeyRepeats = false

  private init() {
    krbn_set_hid_capture_mouse_wheels(captureMouseWheels)
  }
}
