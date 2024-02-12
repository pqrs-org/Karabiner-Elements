import SwiftUI

private func callback(_ filePath: UnsafePointer<Int8>?, _ context: UnsafeMutableRawPointer?) {
  if filePath == nil { return }

  let path = String(cString: filePath!)
  guard let text = try? String(contentsOfFile: path, encoding: .utf8) else { return }

  Task { @MainActor in
    DevicesJsonString.shared.text = text
  }
}

public class DevicesJsonString: ObservableObject {
  public static let shared = DevicesJsonString()

  @Published var text = ""

  private init() {
    libkrbn_enable_device_details_json_file_monitor(callback, nil)
  }

  deinit {
    libkrbn_disable_device_details_json_file_monitor()
  }
}
