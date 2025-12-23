import AsyncAlgorithms
import Combine
import Foundation
import SwiftUI

private func frontmostApplicationHistoryReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  let s = String(cString: jsonString)

  Task { @MainActor in
    LibKrbn.FrontmostApplicationHistory.shared.update(s)
  }
}

extension LibKrbn {
  private struct FrontmostApplicationHistoryEntryPayload: Decodable {
    let bundleIdentifier: String?
    let bundlePath: String?
    let filePath: String?
    let pid: Int?
  }

  struct FrontmostApplicationHistoryEntry: Identifiable, Equatable {
    public let id = UUID()

    let bundleIdentifier: String
    let filePath: String

    public static func == (
      lhs: FrontmostApplicationHistoryEntry,
      rhs: FrontmostApplicationHistoryEntry,
    ) -> Bool {
      lhs.id == rhs.id
    }
  }

  @MainActor
  final class FrontmostApplicationHistory: ObservableObject {
    static let shared = FrontmostApplicationHistory()

    private let timer: AsyncTimerSequence<ContinuousClock>
    private var timerTask: Task<Void, Never>?

    private(set) var jsonString = ""
    @Published var entries: [FrontmostApplicationHistoryEntry] = []

    init() {
      timer = AsyncTimerSequence(
        interval: .milliseconds(1000),
        clock: .continuous
      )
    }

    // We register the callback in the `watch` method rather than in `init`.
    // If libkrbn_register_*_callback is called within init,
    // there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

    public func watch() {
      if timerTask != nil {
        return
      }

      libkrbn_register_console_user_server_client_frontmost_application_history_received_callback(
        frontmostApplicationHistoryReceivedCallback)

      timerTask = Task { @MainActor in
        libkrbn_console_user_server_client_async_get_frontmost_application_history()

        for await _ in timer {
          libkrbn_console_user_server_client_async_get_frontmost_application_history()
        }
      }
    }

    public func update(_ jsonString: String) {
      if self.jsonString == jsonString {
        return
      }
      self.jsonString = jsonString

      guard let data = jsonString.data(using: .utf8) else { return }
      let decoder = JSONDecoder()
      decoder.keyDecodingStrategy = .convertFromSnakeCase
      let encoder = JSONEncoder()
      encoder.keyEncodingStrategy = .convertToSnakeCase

      do {
        let payloads = try decoder.decode(
          [FrontmostApplicationHistoryEntryPayload].self, from: data)

        entries = payloads.filter { payload in
          if payload.bundleIdentifier == "org.pqrs.Karabiner-EventViewer" {
            return false
          }
          if payload.bundleIdentifier == nil && payload.filePath == nil {
            return false
          }
          return true
        }.map { payload in
          FrontmostApplicationHistoryEntry(
            bundleIdentifier: payload.bundleIdentifier ?? "",
            filePath: payload.filePath ?? ""
          )
        }.reversed()
      } catch {
        print(error.localizedDescription)
        return
      }
    }
  }
}
