import SwiftUI

func hidValueMonitorStoppedCallback() {
  Task { @MainActor in
    EventHistory.shared.resetModifierFlags()
  }
}

func hidValueArrivedCallback(
  _ deviceId: UInt64,
  _ usagePage: Int32,
  _ usage: Int32,
  _ integerValue: Int64,
  _ momentarySwitchEventJsonStringPointer: UnsafePointer<CChar>?,
  _ modifierFlagNamePointer: UnsafePointer<CChar>?
) {
  //
  // Skip specific events
  //

  // usage_page::undefined
  if usagePage == 0 {
    return
  }

  // usage::undefined
  if usage == 0 {
    return
  }

  // usage::generic_desktop unknown
  if usagePage == 0x1, usage == -1 {
    return
  }

  // usage::generic_desktop::x
  if usagePage == 0x1, usage == 0x30 {
    return
  }

  // usage::generic_desktop::y
  if usagePage == 0x1, usage == 0x31 {
    return
  }

  // usage::generic_desktop::z
  if usagePage == 0x1, usage == 0x32 {
    return
  }

  // usage::generic_desktop::rz
  if usagePage == 0x1, usage == 0x35 {
    return
  }

  // usage::generic_desktop::wheel
  if usagePage == 0x1, usage == 0x38 {
    return
  }

  // usage::generic_desktop::hat_switch
  if usagePage == 0x1, usage == 0x39 {
    return
  }

  // usage::keyboard_or_keypad::error_rollover
  if usagePage == 0x7, usage == 0x1 {
    return
  }

  // usage::keyboard_or_keypad::post_fail
  if usagePage == 0x7, usage == 0x2 {
    return
  }

  // usage::keyboard_or_keypad::error_undefined
  if usagePage == 0x7, usage == 0x3 {
    return
  }

  // usage::keyboard_or_keypad unknown
  if usagePage == 0x7, usage == -1 {
    return
  }

  // usage::consumer::ac_pan (Horizontal mouse wheel)
  if usagePage == 0xC, usage == 0x238 {
    return
  }

  // usage::consumer unknown
  if usagePage == 0xC, usage == -1 {
    return
  }

  // usage::apple_vendor_top_case unknown
  if usagePage == 0xFF, usage == -1 {
    return
  }

  // usage::apple_vendor_keyboard unknown
  if usagePage == 0xFF01, usage == -1 {
    return
  }

  let momentarySwitchEventJsonString = momentarySwitchEventJsonStringPointer.map {
    String(cString: $0)
  }
  let modifierFlagName = modifierFlagNamePointer.map {
    String(cString: $0)
  }

  //
  // Add entry
  //

  let timestamp = Date()

  Task { @MainActor in
    let entry = EventHistoryEntry(timestamp: timestamp)
    entry.deviceId = deviceId
    entry.product = EVCoreServiceDaemonClient.shared.productName(deviceId: deviceId)

    //
    // entry.code
    //

    entry.usagePage = String(
      format: "%5d (0x%04x)",
      usagePage,
      usagePage)
    entry.usage = String(
      format: "%5d (0x%04x)",
      usage,
      usage)
    entry.integerValue = String(
      format: "%5d",
      integerValue)

    //
    // Handle unknown events
    //

    guard let momentarySwitchEventJsonString else {
      EventHistory.shared.appendUnknownEvent(entry)
      return
    }

    //
    // entry.name
    //

    entry.name = momentarySwitchEventJsonString

    //
    // modifierFlags
    //

    if let modifierFlagName {
      if EventHistory.shared.modifierFlags[deviceId] == nil {
        EventHistory.shared.modifierFlags[deviceId] = Set()
      }

      if integerValue != 0 {
        EventHistory.shared.modifierFlags[deviceId]!.insert(modifierFlagName)
      } else {
        EventHistory.shared.modifierFlags[deviceId]!.remove(modifierFlagName)
      }
    }

    //
    // entry.eventType
    //

    if integerValue != 0 {
      entry.eventType = "down"
    } else {
      entry.eventType = "up"
    }

    //
    // entry.misc
    //

    if usagePage == 0x9 {  // usage_page::button
      if !EventHistory.shared.lastPointingButtonModifierFlags.isEmpty {
        entry.misc = "flags \(EventHistory.shared.lastPointingButtonModifierFlags)"
      }
    } else if let set = EventHistory.shared.modifierFlags[deviceId] {
      if set.count > 0 {
        let flags = set.sorted().joined(separator: ", ")
        entry.misc = "flags \(flags)"
      }
    }

    //
    // Add to entries
    //

    EventHistory.shared.append(entry)
  }
}

@MainActor
public class EventHistoryEntry: Identifiable, Equatable {
  nonisolated public let id = UUID()
  public let timestamp: Date
  public var deviceId: UInt64 = 0
  public var eventType = ""
  public var product = ""
  public var usagePage = ""
  public var usage = ""
  public var integerValue = ""
  public var name = ""
  public var misc = ""

  public init(timestamp: Date) {
    self.timestamp = timestamp
  }

  public var timestampString: String {
    EventViewerDateFormatters.timestamp.string(from: timestamp)
  }

  public var iso8601TimestampString: String {
    EventViewerDateFormatters.iso8601.string(from: timestamp)
  }

  nonisolated public static func == (lhs: EventHistoryEntry, rhs: EventHistoryEntry) -> Bool {
    lhs.id == rhs.id
  }
}

@MainActor
public class EventHistory: ObservableObject {
  public static let shared = EventHistory()

  // Keep maxCount small since too many entries causes performance issue at SwiftUI rendering.
  private let maxCount = 128
  private var startCount = 0
  private var paused = false
  private var rawInputEventsKeyDownMonitor: Any?
  public var modifierFlags: [UInt64: Set<String>] = [:]

  private var pointingButtonModifierFlagsLocalMonitor: Any?
  private var pointingButtonModifierFlagsGlobalMonitor: Any?
  public private(set) var lastPointingButtonModifierFlags: String = ""

  @Published var entries: [EventHistoryEntry] = []
  @Published var unknownEventEntries: [EventHistoryEntry] = []
  @Published private(set) var inputEventsCapturing = false
  @Published private(set) var rawInputEventsCapturing = false
  @Published private(set) var rawInputEventsSelectedDeviceId: UInt64?

  public func startInputEventsCapture() {
    guard !inputEventsCapturing else {
      return
    }

    stopRawInputEventsCapture()
    InputReportHistory.shared.stopCapture()
    clear()
    resetModifierFlags()
    inputEventsCapturing = true
    TemporarilyIgnoredDeviceManager.shared.activate(
      owner: .inputEvents,
      deviceId: nil)
  }

  public func stopInputEventsCapture() {
    TemporarilyIgnoredDeviceManager.shared.deactivate(owner: .inputEvents)
    inputEventsCapturing = false
  }

  public func startRawInputEventsCapture() {
    guard let rawInputEventsSelectedDeviceId else {
      return
    }

    stopInputEventsCapture()
    InputReportHistory.shared.stopCapture()
    clear()
    resetModifierFlags()
    rawInputEventsCapturing = true
    startRawInputEventsKeyDownMonitor()
    TemporarilyIgnoredDeviceManager.shared.activate(
      owner: .rawInputEvents,
      deviceId: rawInputEventsSelectedDeviceId)
  }

  public func stopRawInputEventsCapture() {
    stopRawInputEventsKeyDownMonitor()
    TemporarilyIgnoredDeviceManager.shared.deactivate(owner: .rawInputEvents)
    rawInputEventsCapturing = false
  }

  public func selectRawInputEventsDevice(_ deviceId: UInt64?) {
    guard deviceId != rawInputEventsSelectedDeviceId else {
      return
    }

    stopRawInputEventsCapture()
    rawInputEventsSelectedDeviceId = deviceId
    clear()
    resetModifierFlags()
  }

  public func rawInputEventsSelectedDeviceDisconnected() {
    stopRawInputEventsCapture()
    selectRawInputEventsDevice(nil)
  }

  public func startUnknownEventsMonitoring() {
    stopInputEventsCapture()
    stopRawInputEventsCapture()
    InputReportHistory.shared.stopCapture()
    TemporarilyIgnoredDeviceManager.shared.activate(
      owner: .unknownEvents,
      deviceId: nil)
  }

  public func stopUnknownEventsMonitoring() {
    TemporarilyIgnoredDeviceManager.shared.deactivate(owner: .unknownEvents)
  }

  public func start() {
    startCount += 1
    if startCount == 1 {
      startPointingButtonModifierFlagsMonitors()

      paused = false
    }
  }

  public func stop() {
    startCount -= 1
    if startCount == 0 {
      stopPointingButtonModifierFlagsMonitors()
    }
  }

  public func pause(_ value: Bool) {
    paused = value
  }

  public func resetModifierFlags() {
    modifierFlags.removeAll()
    lastPointingButtonModifierFlags = ""
  }

  public func append(_ entry: EventHistoryEntry) {
    if paused {
      return
    }

    if inputEventsCapturing {
      // Capture values from all devices that EventViewer can open.
    } else if rawInputEventsCapturing,
      entry.deviceId == rawInputEventsSelectedDeviceId
    {
      // Capture values from the selected raw device.
    } else {
      return
    }

    entries.append(entry)
    if entries.count > maxCount {
      entries.removeFirst()
    }
  }

  public func clear() {
    entries.removeAll()
  }

  public func copyToPasteboardJSON() {
    var string = "[\n"

    entries.forEach { entry in
      if entry.eventType.count > 0 {
        if string != "[\n" {
          string += ",\n"
        }

        string += "  {\n"
        string += "    \"timestamp\": \"\(entry.iso8601TimestampString)\",\n"
        string += "    \"type\": \"\(entry.eventType)\",\n"
        string += "    \"name\": \(entry.name),\n"
        string += "    \"usagePage\": \"\(entry.usagePage)\",\n"
        string += "    \"usage\": \"\(entry.usage)\",\n"
        string += "    \"misc\": \"\(entry.misc)\"\n"
        string += "  }"
      }
    }

    string += "\n"
    string += "]"

    copyToPasteboard(string)
  }

  public func copyToPasteboardTSV() {
    var lines = ["Timestamp\tType\tName\tUsage page\tUsage\tMisc"]

    for entry in entries where !entry.eventType.isEmpty {
      let columns = [
        entry.iso8601TimestampString,
        entry.eventType,
        entry.name,
        entry.usagePage,
        entry.usage,
        entry.misc,
      ]
      lines.append(columns.map(tsvCell).joined(separator: "\t"))
    }

    copyToPasteboard(lines.joined(separator: "\n") + "\n")
  }

  private func tsvCell(_ value: String) -> String {
    value
      .replacingOccurrences(of: "\t", with: " ")
      .replacingOccurrences(of: "\r\n", with: " ")
      .replacingOccurrences(of: "\n", with: " ")
      .replacingOccurrences(of: "\r", with: " ")
  }

  private func copyToPasteboard(_ string: String) {
    let pboard = NSPasteboard.general
    pboard.clearContents()
    pboard.writeObjects([string as NSString])
  }

  private func startRawInputEventsKeyDownMonitor() {
    stopRawInputEventsKeyDownMonitor()

    rawInputEventsKeyDownMonitor = NSEvent.addLocalMonitorForEvents(matching: .keyDown) {
      (event: NSEvent) -> NSEvent? in
      if event.keyCode == 53 {  // Escape
        Task { @MainActor in
          EventHistory.shared.stopRawInputEventsCapture()
        }
        return nil
      }

      return event
    }
  }

  private func stopRawInputEventsKeyDownMonitor() {
    if let rawInputEventsKeyDownMonitor {
      NSEvent.removeMonitor(rawInputEventsKeyDownMonitor)
      self.rawInputEventsKeyDownMonitor = nil
    }
  }

  //
  // NSEvent modifier flags handling
  //

  private func startPointingButtonModifierFlagsMonitors() {
    stopPointingButtonModifierFlagsMonitors()

    pointingButtonModifierFlagsLocalMonitor = NSEvent.addLocalMonitorForEvents(
      matching: .flagsChanged
    ) { event in
      Task { @MainActor in
        self.handlePointingButtonModifierFlagsChanged(event)
      }
      return event
    }

    pointingButtonModifierFlagsGlobalMonitor = NSEvent.addGlobalMonitorForEvents(
      matching: .flagsChanged
    ) { event in
      Task { @MainActor in
        self.handlePointingButtonModifierFlagsChanged(event)
      }
    }
  }

  private func stopPointingButtonModifierFlagsMonitors() {
    if let monitor = pointingButtonModifierFlagsLocalMonitor {
      NSEvent.removeMonitor(monitor)
      pointingButtonModifierFlagsLocalMonitor = nil
    }

    if let monitor = pointingButtonModifierFlagsGlobalMonitor {
      NSEvent.removeMonitor(monitor)
      pointingButtonModifierFlagsGlobalMonitor = nil
    }

    lastPointingButtonModifierFlags = ""
  }

  @MainActor
  private func handlePointingButtonModifierFlagsChanged(_ event: NSEvent) {
    let flags = event.modifierFlags.intersection(.deviceIndependentFlagsMask)
    let flagsString = modifierFlagsString(flags)
    if lastPointingButtonModifierFlags == flagsString {
      return
    }

    lastPointingButtonModifierFlags = flagsString
  }

  private func modifierFlagsString(_ flags: NSEvent.ModifierFlags) -> String {
    var names: [String] = []
    if flags.contains(.capsLock) {
      names.append("caps")
    }
    if flags.contains(.shift) {
      names.append("shift")
    }
    if flags.contains(.control) {
      names.append("ctrl")
    }
    if flags.contains(.option) {
      names.append("opt")
    }
    if flags.contains(.command) {
      names.append("cmd")
    }
    if flags.contains(.numericPad) {
      names.append("numpad")
    }
    if flags.contains(.help) {
      names.append("help")
    }
    if flags.contains(.function) {
      names.append("fn")
    }

    return names.joined(separator: ", ")
  }

  //
  // Unknown Events
  //

  public func appendUnknownEvent(_ entry: EventHistoryEntry) {
    if paused {
      return
    }

    unknownEventEntries.append(entry)
    if unknownEventEntries.count > maxCount {
      unknownEventEntries.removeFirst()
    }
  }

  public func clearUnknownEvents() {
    unknownEventEntries.removeAll()
  }

  public func copyToPasteboardUnknownEvents() {
    var string = "[\n"

    unknownEventEntries.forEach { entry in
      if string != "[\n" {
        string += ",\n"
      }

      string += "  {\n"
      string += "    \"timestamp\": \"\(entry.iso8601TimestampString)\",\n"
      string += "    \"value\": \"\(entry.integerValue)\",\n"
      string += "    \"usagePage\": \"\(entry.usagePage)\",\n"
      string += "    \"usage\": \"\(entry.usage)\"\n"
      string += "  }"
    }

    string += "\n"
    string += "]"

    let pboard = NSPasteboard.general
    pboard.clearContents()
    pboard.writeObjects([string as NSString])
  }
}
