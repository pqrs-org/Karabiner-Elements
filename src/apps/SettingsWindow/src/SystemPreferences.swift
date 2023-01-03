import Combine
import Foundation
import SwiftUI

private func callback(
  _ systemPreferencesProperties: UnsafePointer<libkrbn_system_preferences_properties>?,
  _ context: UnsafeMutableRawPointer?
) {
  if systemPreferencesProperties == nil { return }
  if context == nil { return }

  let obj: SystemPreferences! = unsafeBitCast(context, to: SystemPreferences.self)
  let systemPreferencesPropertiesCopy = systemPreferencesProperties!.pointee
  DispatchQueue.main.async { [weak obj, systemPreferencesPropertiesCopy] in
    guard let obj = obj else { return }

    var properties = systemPreferencesPropertiesCopy
    obj.updateProperties(&properties)
  }
}

final class SystemPreferences: ObservableObject {
  static let shared = SystemPreferences()

  // This variable will be used in VirtualKeyboardView in order to show "log out required" message.
  @Published var keyboardTypeChanged = false

  private var didSetEnabled = false

  private init() {
    didSetEnabled = true

    let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
    libkrbn_enable_system_preferences_monitor(callback, obj)
  }

  public func updateProperties(
    _ systemPreferencesProperties: inout libkrbn_system_preferences_properties
  ) {
    didSetEnabled = false

    useFkeysAsStandardFunctionKeys =
      systemPreferencesProperties.use_fkeys_as_standard_function_keys

    let keyboardTypesSize = libkrbn_system_preferences_properties_get_keyboard_types_size()
    withUnsafePointer(to: &(systemPreferencesProperties.keyboard_types.0)) {
      keyboardTypesPtr in
      var newKeyboardTypes: [LibKrbn.KeyboardType] = []
      for i in 0..<keyboardTypesSize {
        newKeyboardTypes.append(LibKrbn.KeyboardType(i, Int(keyboardTypesPtr[i])))
      }
      keyboardTypes = newKeyboardTypes
    }

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
