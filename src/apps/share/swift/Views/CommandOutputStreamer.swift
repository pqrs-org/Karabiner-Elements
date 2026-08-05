import Foundation
import SwiftUI

@MainActor
final class CommandOutputStreamer: ObservableObject {
  @Published private(set) var isTextReady = false
  @Published private(set) var text = ""

  private var task: Task<Void, Never>?

  func start(
    launchPath: String,
    arguments: [String],
    environment: [String: String]? = nil
  ) {
    guard task == nil else { return }

    task = Task.detached { [weak self] in
      guard let self else { return }

      let command = Process()
      command.launchPath = launchPath
      command.arguments = arguments
      if let environment {
        command.environment = environment
      }

      let pipe = Pipe()
      command.standardOutput = pipe

      let handle = pipe.fileHandleForReading

      handle.readabilityHandler = { [weak self] h in
        guard let self else { return }
        let data = h.availableData
        if data.isEmpty {
          return
        }

        Task {
          await MainActor.run {
            if let text = String(bytes: data, encoding: .utf8) {
              self.appendText(text)
            }
          }
        }
      }

      command.launch()
      command.waitUntilExit()

      handle.readabilityHandler = nil

      // Read remaining data to end and finalize.
      if let tail = try? handle.readToEnd() {
        await MainActor.run {
          if let text = String(bytes: tail, encoding: .utf8) {
            self.appendText(text)
          }
        }
      }

      await MainActor.run {
        self.task = nil
      }
    }
  }

  func clear() {
    text = ""
    isTextReady = false
  }

  private func appendText(_ text: String) {
    self.text += text
    isTextReady = true
  }
}
