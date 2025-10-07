import SwiftUI

@MainActor
public class SystemExtensions {
  public static let shared = SystemExtensions()

  let stream = RealtimeTextStream()

  public func update() {
    let command = Process()
    command.launchPath = "/usr/bin/systemextensionsctl"
    command.arguments = [
      "list"
    ]

    command.environment = [
      "LC_ALL": "C"
    ]

    let pipe = Pipe()
    command.standardOutput = pipe

    command.launch()
    command.waitUntilExit()

    if let data = try? pipe.fileHandleForReading.readToEnd() {
      stream.setText(String(data: data, encoding: .utf8) ?? "")
    }
  }
}
