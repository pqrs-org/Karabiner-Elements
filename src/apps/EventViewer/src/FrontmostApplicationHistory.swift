import AsyncAlgorithms
import Combine
import Foundation

func frontmostApplicationHistoryReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  let text = String(cString: jsonString)

  Task { @MainActor in
    FrontmostApplicationHistory.shared.update(text)
  }
}

private struct FrontmostApplicationHistoryEntryPayload: Decodable {
  let bundleIdentifier: String?
  let bundlePath: String?
  let filePath: String?
  let pid: Int?
  let detectionSource: String?
}

struct FrontmostApplicationHistoryEntry: Identifiable, Equatable {
  let id = UUID()

  let bundleIdentifier: String
  let filePath: String
  let detectionSource: String

  static func == (
    lhs: FrontmostApplicationHistoryEntry,
    rhs: FrontmostApplicationHistoryEntry
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

  func watch() {
    if timerTask != nil {
      return
    }

    timerTask = Task { @MainActor in
      krbn_console_user_server_async_get_frontmost_application_history()

      for await _ in timer {
        krbn_console_user_server_async_get_frontmost_application_history()
      }
    }
  }

  func update(_ jsonString: String) {
    if self.jsonString == jsonString {
      return
    }
    self.jsonString = jsonString

    guard let data = jsonString.data(using: .utf8) else { return }
    let decoder = JSONDecoder()
    decoder.keyDecodingStrategy = .convertFromSnakeCase

    do {
      let payloads = try decoder.decode(
        [FrontmostApplicationHistoryEntryPayload].self,
        from: data)

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
          filePath: payload.filePath ?? "",
          detectionSource: payload.detectionSource ?? ""
        )
      }.reversed()
    } catch {
      print(error.localizedDescription)
    }
  }
}
