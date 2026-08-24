import AppKit
import Foundation

func hidInputReportArrivedCallback(
  _ deviceId: UInt64,
  _ reportId: UInt32,
  _ bytes: UnsafePointer<UInt8>?,
  _ length: Int
) {
  let report =
    bytes.map {
      Array(UnsafeBufferPointer(start: $0, count: length))
    } ?? []
  let timestamp = Date()

  Task { @MainActor in
    InputReportHistory.shared.append(
      InputReportEntry(
        timestamp: timestamp,
        deviceId: deviceId,
        reportId: reportId,
        bytes: report))
  }
}

func hidDeviceOpenStateChangedCallback(_ deviceId: UInt64, _ opened: Bool) {
  Task { @MainActor in
    EVCoreServiceDaemonClient.shared.setHIDDevice(deviceId, opened: opened)
  }
}

@MainActor
final class InputReportEntry: Identifiable, Equatable {
  nonisolated let id = UUID()
  let timestamp: Date
  let deviceId: UInt64
  let reportId: UInt32
  let bytes: [UInt8]

  init(timestamp: Date, deviceId: UInt64, reportId: UInt32, bytes: [UInt8]) {
    self.timestamp = timestamp
    self.deviceId = deviceId
    self.reportId = reportId
    self.bytes = bytes
  }

  var timestampString: String {
    EventViewerDateFormatters.timestamp.string(from: timestamp)
  }

  var iso8601TimestampString: String {
    EventViewerDateFormatters.iso8601.string(from: timestamp)
  }

  var reportIdString: String {
    String(reportId)
  }

  var bytesString: String {
    bytes.map { String(format: "%02x", $0) }.joined(separator: " ")
  }

  nonisolated static func == (lhs: InputReportEntry, rhs: InputReportEntry) -> Bool {
    lhs.id == rhs.id
  }
}

@MainActor
final class InputReportHistory: ObservableObject {
  static let shared = InputReportHistory()

  private let maxCount = 128

  @Published private(set) var entries: [InputReportEntry] = []

  func append(_ entry: InputReportEntry) {
    guard CaptureCoordinator.shared.acceptsInputReport(deviceId: entry.deviceId) else {
      return
    }

    entries.append(entry)
    if entries.count > maxCount {
      entries.removeFirst(entries.count - maxCount)
    }
  }

  func clear() {
    entries.removeAll()
  }

  func copyToPasteboardJSON() {
    let objects: [[String: Any]] = entries.map { entry in
      [
        "timestamp": entry.iso8601TimestampString,
        "report_id": entry.reportId,
        "data": entry.bytesString,
      ]
    }

    guard
      let data = try? JSONSerialization.data(
        withJSONObject: objects,
        options: [.prettyPrinted, .sortedKeys]),
      var text = String(data: data, encoding: .utf8)
    else {
      return
    }
    text += "\n"

    copyToPasteboard(text)
  }

  func copyToPasteboardTSV() {
    var lines = ["Timestamp\tReport ID\tData"]

    for entry in entries {
      lines.append(
        [
          entry.iso8601TimestampString,
          String(entry.reportId),
          entry.bytesString,
        ].joined(separator: "\t")
      )
    }

    copyToPasteboard(lines.joined(separator: "\n") + "\n")
  }

  private func copyToPasteboard(_ text: String) {
    let pasteboard = NSPasteboard.general
    pasteboard.clearContents()
    pasteboard.writeObjects([text as NSString])
  }

}
