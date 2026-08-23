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
  private static let timestampFormatter: DateFormatter = {
    let formatter = DateFormatter()
    formatter.locale = Locale(identifier: "en_US_POSIX")
    formatter.dateFormat = "HH:mm:ss.SSS"
    return formatter
  }()
  private static let iso8601TimestampFormatter: ISO8601DateFormatter = {
    let formatter = ISO8601DateFormatter()
    formatter.formatOptions = [.withInternetDateTime, .withFractionalSeconds]
    formatter.timeZone = .current
    return formatter
  }()

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
    Self.timestampFormatter.string(from: timestamp)
  }

  var iso8601TimestampString: String {
    Self.iso8601TimestampFormatter.string(from: timestamp)
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
  private var keyDownMonitor: Any?

  @Published private(set) var capturing = false
  @Published private(set) var selectedDeviceId: UInt64?
  @Published private(set) var entries: [InputReportEntry] = []

  func startCapture() {
    guard let selectedDeviceId else {
      return
    }

    clear()
    capturing = true
    startKeyDownMonitor()
    TemporarilyIgnoredDeviceManager.shared.activate(
      owner: .captureInputReports,
      deviceId: selectedDeviceId)
    krbn_set_hid_input_report_capture_device(selectedDeviceId)
  }

  func stopCapture() {
    stopKeyDownMonitor()
    krbn_set_hid_input_report_capture_device(0)
    TemporarilyIgnoredDeviceManager.shared.deactivate(owner: .captureInputReports)
    capturing = false
  }

  func selectDevice(_ deviceId: UInt64?) {
    clear()
    selectedDeviceId = deviceId

    if capturing, let deviceId {
      TemporarilyIgnoredDeviceManager.shared.update(
        owner: .captureInputReports,
        deviceId: deviceId)
      krbn_set_hid_input_report_capture_device(deviceId)
      startKeyDownMonitor()
    } else {
      krbn_set_hid_input_report_capture_device(0)
      stopKeyDownMonitor()
    }
  }

  func deviceDisconnected() {
    if capturing {
      stopCapture()
    }
    selectDevice(nil)
  }

  func append(_ entry: InputReportEntry) {
    guard capturing, entry.deviceId == selectedDeviceId else {
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

  private func startKeyDownMonitor() {
    stopKeyDownMonitor()

    keyDownMonitor = NSEvent.addLocalMonitorForEvents(matching: .keyDown) {
      (event: NSEvent) -> NSEvent? in
      if event.keyCode == 53 {  // Escape
        Task { @MainActor in
          InputReportHistory.shared.stopCapture()
        }
      }

      // The raw HID report has already been captured before the event reaches AppKit.
      // Discard it here so test input does not operate the EventViewer interface.
      return nil
    }
  }

  private func stopKeyDownMonitor() {
    if let keyDownMonitor {
      NSEvent.removeMonitor(keyDownMonitor)
      self.keyDownMonitor = nil
    }
  }
}
