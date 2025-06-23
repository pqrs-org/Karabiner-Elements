import Combine
import Foundation
import SwiftUI

private func callback() {
  Task { @MainActor in
    SystemPreferences.shared.updateProperties()
  }
}

@MainActor
final class SystemPreferences: ObservableObject {
  static let shared = SystemPreferences()

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

    didSetEnabled = true
  }

  @Published var useFkeysAsStandardFunctionKeys: Bool = false
}
