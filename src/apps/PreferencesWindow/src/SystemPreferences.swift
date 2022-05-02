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
  obj.updateProperties(systemPreferencesPropertiesCopy)
}

final class SystemPreferences: ObservableObject {
  static let shared = SystemPreferences()

  private var didSetEnabled = false

  private init() {
    didSetEnabled = true

    let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
    libkrbn_enable_system_preferences_monitor(callback, obj)
  }

  public func updateProperties(
    _ systemPreferencesProperties: libkrbn_system_preferences_properties
  ) {
    didSetEnabled = false

    useFkeysAsStandardFunctionKeys = systemPreferencesProperties.use_fkeys_as_standard_function_keys

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
}
