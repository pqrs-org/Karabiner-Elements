import SwiftUI

private func callback() {
  Task { @MainActor in
    guard
      let text = try? String(
        contentsOfFile: DevicesJsonString.shared.devicesJsonFilePath,
        encoding: .utf8
      )
    else { return }

    DevicesJsonString.shared.stream.setText(text)
  }
}

@MainActor
public class DevicesJsonString {
  public static let shared = DevicesJsonString()

  let devicesJsonFilePath = LibKrbn.devicesJsonFilePath()

  let stream = RealtimeTextStream()

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    libkrbn_enable_file_monitors()

    if let cString = devicesJsonFilePath.cString(using: .utf8) {
      libkrbn_register_file_updated_callback(cString, callback)
      libkrbn_enqueue_callback(callback)
    }
  }

  public func stop() {
    if let cString = devicesJsonFilePath.cString(using: .utf8) {
      libkrbn_unregister_file_updated_callback(cString, callback)
    }

    // We don't call `libkrbn_disable_file_monitors` because the file monitors may be used elsewhere.
  }
}
