import SwiftUI

func hidValueMonitorStoppedCallback() {
  Task { @MainActor in
    EventHistory.shared.resetModifierFlags()
  }
}

func hidValueArrivedCallback(
  _ deviceId: UInt64,
  _ isVirtualKeyboard: Bool,
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

  let mouseWheelName: String?
  if usagePage == 0x1, usage == 0x38 {
    mouseWheelName = "vertical_wheel"
  } else if usagePage == 0xC, usage == 0x238 {
    mouseWheelName = "horizontal_wheel"
  } else {
    mouseWheelName = nil
  }

  //
  // Add entry
  //

  let timestamp = Date()
  guard let captureToken = CaptureEventValidator.shared.inputEventToken(deviceId: deviceId) else {
    return
  }

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
    // The virtual keyboard reports LED state changes as input for LED synchronization.
    // These notifications cannot be remapped.
    //

    if isVirtualKeyboard, usagePage == 0x08 {
      switch usage {
      case 0x01:
        entry.name = "num lock LED"
      case 0x02:
        entry.name = "caps lock LED"
      default:
        entry.name = "unsupported LED"
      }
      entry.nameJSON = "\"\(entry.name)\""
      entry.eventType = integerValue != 0 ? "on" : "off"
      entry.misc = "State notification (cannot be remapped)"
      entry.isStateNotification = true
      EventHistory.shared.append(entry, captureToken: captureToken)
      return
    }

    //
    // Handle mouse wheels
    //

    if let mouseWheelName {
      entry.eventType = "scroll"
      entry.name = mouseWheelName
      entry.nameJSON = "\"\(mouseWheelName)\""
      entry.isMouseWheel = true

      if !EventHistory.shared.lastPointingButtonModifierFlags.isEmpty {
        entry.misc = "flags \(EventHistory.shared.lastPointingButtonModifierFlags)"
      }

      EventHistory.shared.append(entry, captureToken: captureToken)
      return
    }

    //
    // Handle unknown events
    //

    guard let momentarySwitchEventJsonString else {
      entry.isUnknownEvent = true
      EventHistory.shared.append(entry, captureToken: captureToken)
      return
    }

    //
    // entry.name
    //

    entry.name = momentarySwitchEventJsonString
    entry.nameJSON = momentarySwitchEventJsonString

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

    EventHistory.shared.append(entry, captureToken: captureToken)
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
  public var nameJSON = ""
  public var misc = ""
  public var isMouseWheel = false
  public var isUnknownEvent = false
  public var isStateNotification = false

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
  public var modifierFlags: [UInt64: Set<String>] = [:]

  private var pointingButtonModifierFlagsLocalMonitor: Any?
  private var pointingButtonModifierFlagsGlobalMonitor: Any?
  public private(set) var lastPointingButtonModifierFlags: String = ""

  @Published var entries: [EventHistoryEntry] = []

  public func resetModifierFlags() {
    modifierFlags.removeAll()
    lastPointingButtonModifierFlags = ""
  }

  func append(_ entry: EventHistoryEntry, captureToken: CaptureEventToken) {
    if entry.isMouseWheel && !CaptureSettings.shared.captureMouseWheels {
      return
    }

    if entry.isUnknownEvent && !CaptureSettings.shared.captureUnknownEvents {
      return
    }

    if !CaptureEventValidator.shared.isCurrent(captureToken) {
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
      if string != "[\n" {
        string += ",\n"
      }

      if entry.isUnknownEvent {
        string += "  {\n"
        string += "    \"timestamp\": \"\(entry.iso8601TimestampString)\",\n"
        string += "    \"type\": \"unknown\",\n"
        string += "    \"value\": \"\(entry.integerValue)\",\n"
        string += "    \"usagePage\": \"\(entry.usagePage)\",\n"
        string += "    \"usage\": \"\(entry.usage)\"\n"
        string += "  }"
      } else {
        string += "  {\n"
        string += "    \"timestamp\": \"\(entry.iso8601TimestampString)\",\n"
        string += "    \"type\": \"\(entry.eventType)\",\n"
        string += "    \"name\": \(entry.nameJSON),\n"
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

    for entry in entries {
      let columns = [
        entry.iso8601TimestampString,
        entry.isUnknownEvent ? "unknown" : entry.eventType,
        entry.isUnknownEvent ? "Unsupported HID usage" : entry.name,
        entry.usagePage,
        entry.usage,
        entry.isUnknownEvent ? "integer value: \(entry.integerValue)" : entry.misc,
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

  //
  // NSEvent modifier flags handling
  //

  func startPointingButtonModifierFlagsMonitoring() {
    stopPointingButtonModifierFlagsMonitoring()

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

  func stopPointingButtonModifierFlagsMonitoring() {
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
}
