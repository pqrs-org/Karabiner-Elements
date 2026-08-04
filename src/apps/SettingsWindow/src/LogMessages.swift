import AsyncAlgorithms
import SwiftUI

enum LogLevel {
  case debug
  case info
  case warn
  case error
}

private struct LogMessagePayload: Decodable {
  let text: String
  let logLevel: String
  let dateNumber: UInt64
}

private func callback(
  _ json: UnsafePointer<CChar>,
  _ length: Int
) {
  let data = Data(bytes: json, count: length)

  Task { @MainActor in
    let decoder = JSONDecoder()
    decoder.keyDecodingStrategy = .convertFromSnakeCase

    do {
      let payloads = try decoder.decode([LogMessagePayload].self, from: data)
      let entries = payloads.map {
        let logLevel: LogLevel =
          switch $0.logLevel {
          case "debug": .debug
          case "warn": .warn
          case "error": .error
          default: .info
          }

        return LogMessageEntry(
          text: $0.text,
          logLevel: logLevel,
          dateNumber: $0.dateNumber)
      }
      LogMessages.shared.setEntries(entries)
    } catch {
      print("Failed to decode log lines JSON: \(error)")
      LogMessages.shared.setEntries([])
    }
  }
}

@MainActor
public class LogMessageEntry: Identifiable, Equatable {
  nonisolated public let id: String
  public var text = ""
  let logLevel: LogLevel
  public var dateNumber: UInt64
  public var foregroundColor = Color.primary
  public var backgroundColor = Color.clear

  init(text: String, logLevel: LogLevel, dateNumber: UInt64) {
    id = "\(dateNumber):\(logLevel):\(text)"
    self.text = text
    self.logLevel = logLevel
    self.dateNumber = dateNumber

    switch logLevel {
    case LogLevel.debug:
      foregroundColor = Color.debugForeground
      backgroundColor = Color.debugBackground
    case LogLevel.info:
      break
    case LogLevel.warn:
      foregroundColor = Color.warningForeground
      backgroundColor = Color.warningBackground
    case LogLevel.error:
      foregroundColor = Color.errorForeground
      backgroundColor = Color.errorBackground
    }
  }

  nonisolated public static func == (lhs: LogMessageEntry, rhs: LogMessageEntry) -> Bool {
    lhs.id == rhs.id
  }
}

@MainActor
public class LogMessages: ObservableObject {
  public static let shared = LogMessages()

  @Published var entries: [LogMessageEntry] = []
  @Published var currentTimeString = ""

  private var dividers: [LogMessageEntry] = []

  private let timer: AsyncTimerSequence<ContinuousClock>
  private var timerTask: Task<Void, Never>?

  init() {
    timer = AsyncTimerSequence(
      interval: .seconds(1),
      clock: .continuous
    )
  }

  public func watch() {
    entries = []

    krbn_enable_log_monitor()
    krbn_register_log_messages_updated_callback(callback)

    //
    // Create timer
    //

    timerTask = Task { @MainActor in
      self.updateCurrentTimeString()

      for await _ in timer {
        self.updateCurrentTimeString()
      }
    }
  }

  public func unwatch() {
    krbn_disable_log_monitor()

    timerTask?.cancel()
  }

  public func setEntries(_ entries: [LogMessageEntry]) {
    var newEntries: [LogMessageEntry] = []

    //
    // Remove old dividers
    //

    while dividers.count > 0,
      entries.count > 0,
      dividers[0].dateNumber < entries[0].dateNumber
    {
      dividers.removeFirst()
    }

    //
    // Merge entries and dividers
    //

    var dividerIndex = 0
    entries.forEach { e in
      while dividerIndex < dividers.count,
        dividers[dividerIndex].dateNumber < e.dateNumber
      {
        newEntries.append(dividers[dividerIndex])
        dividerIndex += 1
      }
      newEntries.append(e)
    }

    while dividerIndex < dividers.count {
      newEntries.append(dividers[dividerIndex])
      dividerIndex += 1
    }

    self.entries = newEntries
  }

  public func addDivider() {
    let formatter = DateFormatter()
    formatter.locale = Locale(identifier: "en_US_POSIX")
    formatter.dateFormat = "yyyyMMddHHmmssSSS"
    let dateNumberString = formatter.string(from: Date())

    if let dateNumber = UInt64(dateNumberString) {
      let entry = LogMessageEntry(
        text: String(repeating: "-", count: 80),
        logLevel: LogLevel.info,
        dateNumber: dateNumber)
      entry.foregroundColor = Color(NSColor.textBackgroundColor)
      entry.backgroundColor = Color(NSColor.textColor)

      dividers.append(entry)
      entries.append(entry)
    }
  }

  private func updateCurrentTimeString() {
    let formatter = DateFormatter()
    formatter.locale = Locale(identifier: "en_US_POSIX")
    formatter.dateFormat = "[yyyy-MM-dd HH:mm:ss]"

    self.currentTimeString = formatter.string(from: Date())
  }
}
