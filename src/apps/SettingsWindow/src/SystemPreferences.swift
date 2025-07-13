import Combine
import Foundation
import SwiftUI

private func callback() {
  Task { @MainActor in
    if let jsonData = try? Data(
      contentsOf: URL(
        fileURLWithPath: SystemPreferences.shared.manipulatorEnvironmentJsonFilePath))
    {
      guard
        let any = try? JSONSerialization.jsonObject(
          with: jsonData,
          options: [.allowFragments]
        )
      else {
        return
      }

      guard let root = any as? [String: Any] else {
        return
      }

      if let variables = root["variables"] as? [String: Any],
        let useFkeysAsStandardFunctionKeys = variables["system.use_fkeys_as_standard_function_keys"]
          as? Bool
      {
        SystemPreferences.shared.useFkeysAsStandardFunctionKeys = useFkeysAsStandardFunctionKeys
      }
    }
  }
}

@MainActor
final class SystemPreferences: ObservableObject {
  static let shared = SystemPreferences()

  let manipulatorEnvironmentJsonFilePath = LibKrbn.manipulatorEnvironmentJsonFilePath()

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    libkrbn_enable_file_monitors()

    libkrbn_register_file_updated_callback(
      manipulatorEnvironmentJsonFilePath.cString(using: .utf8),
      callback)
    libkrbn_enqueue_callback(callback)
  }

  public func stop() {
    libkrbn_unregister_file_updated_callback(
      manipulatorEnvironmentJsonFilePath.cString(using: .utf8),
      callback)

    // We don't call `libkrbn_disable_file_monitors` because the file monitors may be used elsewhere.
  }

  @Published var useFkeysAsStandardFunctionKeys: Bool = false
}
