import SwiftUI

private func callback() {
  Task { @MainActor in
    guard
      let text = try? String(
        contentsOfFile: VariablesJsonString.shared.manipulatorEnvironmentJsonFilePath,
        encoding: .utf8
      )
    else { return }

    VariablesJsonString.shared.text = text
  }
}

@MainActor
public class VariablesJsonString: ObservableObject {
  public static let shared = VariablesJsonString()

  let manipulatorEnvironmentJsonFilePath = LibKrbn.manipulatorEnvironmentJsonFilePath()

  @Published var text = ""

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
}
