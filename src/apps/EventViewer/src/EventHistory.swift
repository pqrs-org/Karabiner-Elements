import SwiftUI

private func callback(
  _ deviceId: UInt64,
  _ isKeyboard: Bool,
  _ isPointingDevice: Bool,
  _ isGamePad: Bool,
  _ usagePage: Int32,
  _ usage: Int32,
  _ logicalMax: Int64,
  _ logicalMin: Int64,
  _ integerValue: Int64
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

  //
  // Add entry
  //

  Task { @MainActor in
    let entry = EventHistoryEntry()

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

    if !libkrbn_is_momentary_switch_event_target(usagePage, usage) {
      EventHistory.shared.appendUnknownEvent(entry)
      return
    }

    //
    // entry.name
    //

    var buffer = [Int8](repeating: 0, count: 256)
    libkrbn_get_momentary_switch_event_json_string(&buffer, buffer.count, usagePage, usage)
    let jsonString = String(utf8String: buffer) ?? ""

    entry.name = jsonString

    //
    // modifierFlags
    //

    if libkrbn_is_modifier_flag(usagePage, usage) {
      libkrbn_get_modifier_flag_name(&buffer, buffer.count, usagePage, usage)
      let modifierFlagName = String(utf8String: buffer) ?? ""

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
  public var eventType = ""
  public var usagePage = ""
  public var usage = ""
  public var integerValue = ""
  public var name = ""
  public var misc = ""

  nonisolated public static func == (lhs: EventHistoryEntry, rhs: EventHistoryEntry) -> Bool {
    lhs.id == rhs.id
  }
}

@MainActor
public class EventHistory: ObservableObject {
  public static let shared = EventHistory()

  // Keep maxCount small since too many entries causes performance issue at SwiftUI rendering.
  private let maxCount = 32
  private var startCount = 0
  private var paused = false
  public var modifierFlags: [UInt64: Set<String>] = [:]

  private var pointingButtonModifierFlagsLocalMonitor: Any?
  private var pointingButtonModifierFlagsGlobalMonitor: Any?
  public private(set) var lastPointingButtonModifierFlags: String = ""

  @Published var entries: [EventHistoryEntry] = []
  @Published var unknownEventEntries: [EventHistoryEntry] = []

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    startCount += 1
    if startCount == 1 {
      libkrbn_enable_hid_value_monitor()

      libkrbn_register_hid_value_arrived_callback(callback)

      startPointingButtonModifierFlagsMonitor()

      paused = false
    }
  }

  public func stop() {
    startCount -= 1
    if startCount == 0 {
      libkrbn_disable_hid_value_monitor()

      stopPointingButtonModifierFlagsMonitor()
    }
  }

  public func pause(_ value: Bool) {
    paused = value
  }

  public func observed() -> Bool {
    libkrbn_hid_value_monitor_observed()
  }

  public func append(_ entry: EventHistoryEntry) {
    if paused {
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

  public func copyToPasteboard() {
    var string = "[\n"

    entries.forEach { entry in
      if entry.eventType.count > 0 {
        if string != "[\n" {
          string += ",\n"
        }

        string += "  {\n"
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

    let pboard = NSPasteboard.general
    pboard.clearContents()
    pboard.writeObjects([string as NSString])
  }

  //
  // NSEvent modifier flags handling
  //

  private func startPointingButtonModifierFlagsMonitor() {
    stopPointingButtonModifierFlagsMonitor()

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

  private func stopPointingButtonModifierFlagsMonitor() {
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
      if entry.eventType.count > 0 {
        if string != "[\n" {
          string += ",\n"
        }

        string += "  {\n"
        string += "    \"value\": \"\(entry.eventType)\",\n"
        string += "    \"usagePage\": \"\(entry.usagePage)\",\n"
        string += "    \"usage\": \"\(entry.usage)\"\n"
        string += "  }"
      }
    }

    string += "\n"
    string += "]"

    let pboard = NSPasteboard.general
    pboard.clearContents()
    pboard.writeObjects([string as NSString])
  }
}
