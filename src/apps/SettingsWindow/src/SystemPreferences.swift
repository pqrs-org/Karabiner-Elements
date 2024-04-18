import Combine
import Foundation
import SwiftUI

private func callback() {
  Task { @MainActor in
    SystemPreferences.shared.updateProperties()
  }
}

final class SystemPreferences: ObservableObject {
  static let shared = SystemPreferences()

  // This variable will be used in VirtualKeyboardView in order to show "log out required" message.
  @Published var keyboardTypeChanged = false

  private var didSetEnabled = false

  private init() {
    didSetEnabled = true
  }

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    libkrbn_enable_system_preferences_monitor()

    libkrbn_register_system_preferences_updated_callback(callback)
    libkrbn_enqueue_callback(callback)
  }

  public func updateProperties() {
    didSetEnabled = false

    useFkeysAsStandardFunctionKeys =
      libkrbn_system_preferences_properties_get_use_fkeys_as_standard_function_keys()

    let countryCodesCount = 8
    var newKeyboardTypes: [LibKrbn.KeyboardType] = []
    for i in 0..<countryCodesCount {
      newKeyboardTypes.append(
        LibKrbn.KeyboardType(
          i,
          Int(libkrbn_system_preferences_properties_get_keyboard_type(UInt64(i)))))
    }
    keyboardTypes = newKeyboardTypes

    didSetEnabled = true
  }

  @Published var useFkeysAsStandardFunctionKeys: Bool = false {
    didSet {
      if didSetEnabled {
        var domain =
          UserDefaults.standard.persistentDomain(forName: UserDefaults.globalDomain) ?? [:]
        domain["com.apple.keyboard.fnState"] = useFkeysAsStandardFunctionKeys
        UserDefaults.standard.setPersistentDomain(domain, forName: UserDefaults.globalDomain)
      }
    }
  }

  @Published var keyboardTypes: [LibKrbn.KeyboardType] = [] {
    didSet {
      if didSetEnabled {
        keyboardTypes.forEach { keyboardType in
          LibKrbn.GrabberClient.shared.setKeyboardType(keyboardType)
        }

        keyboardTypeChanged = true
      }
    }
  }
}
