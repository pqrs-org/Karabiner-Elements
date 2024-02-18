import SwiftUI

private func enable() {
  Task { @MainActor in
    LibKrbn.GrabberClient.shared.enabled = true
  }
}

private func disable() {
  Task { @MainActor in
    LibKrbn.GrabberClient.shared.enabled = false
  }
}

extension LibKrbn {
  public class GrabberClient: ObservableObject {
    public static let shared = GrabberClient()

    @Published public fileprivate(set) var enabled = false

    private init() {
      libkrbn_enable_grabber_client(
        enable,
        disable,
        disable)
    }

    public func setAppIcon(_ number: Int32) {
      libkrbn_grabber_client_async_set_app_icon(number)
    }

    public func setKeyboardType(_ keyboardType: LibKrbn.KeyboardType) {
      if keyboardType.keyboardType > 0 {
        libkrbn_grabber_client_async_set_keyboard_type(
          UInt64(keyboardType.countryCode),
          UInt64(keyboardType.keyboardType))
      }
    }
  }
}
