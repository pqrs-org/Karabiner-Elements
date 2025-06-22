import SwiftUI

@MainActor
public class SystemExtensions: ObservableObject {
  public static let shared = SystemExtensions()

  @Published var list = ""

  public func updateList() {
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
      list = String(data: data, encoding: .utf8) ?? ""
    }
  }
}
