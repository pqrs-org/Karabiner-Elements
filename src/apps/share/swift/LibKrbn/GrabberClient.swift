import SwiftUI

private func callback() {
  let status = libkrbn_grabber_client_get_status()

  Task { @MainActor in
    if status == libkrbn_grabber_client_status_connected {
      LibKrbn.GrabberClient.shared.enabled = true
    } else {
      LibKrbn.GrabberClient.shared.enabled = false
    }
  }
}

extension LibKrbn {
  public class GrabberClient: ObservableObject {
    public static let shared = GrabberClient()

    @Published public fileprivate(set) var enabled = false

    private init() {
      libkrbn_enable_grabber_client()
      libkrbn_register_grabber_client_status_changed_callback(callback)
      callback()
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
