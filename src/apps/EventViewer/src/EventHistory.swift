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
      format: "%d (0x%04x)",
      usagePage,
      usagePage)
    entry.usage = String(
      format: "%d (0x%04x)",
      usage,
      usage)

    //
    // Handle unknown events
    //

    if !libkrbn_is_momentary_switch_event_target(usagePage, usage) {
      entry.eventType = "\(integerValue)"
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

    if let set = EventHistory.shared.modifierFlags[deviceId] {
      if set.count > 0 {
        let flags = set.sorted().joined(separator: ",")
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
  public var modifierFlags: [UInt64: Set<String>] = [:]

  @Published var entries: [EventHistoryEntry] = []
  @Published var unknownEventEntries: [EventHistoryEntry] = []
  @Published var monitoring = false

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    libkrbn_enable_hid_value_monitor()

    libkrbn_register_hid_value_arrived_callback(callback)

    monitoring = true
  }

  public func stop() {
    libkrbn_disable_hid_value_monitor()

    monitoring = false
  }

  public func observed() -> Bool {
    libkrbn_hid_value_monitor_observed()
  }

  public func append(_ entry: EventHistoryEntry) {
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
  // Mouse event handling
  //

  func modifierFlagsString(_ flags: NSEvent.ModifierFlags) -> String {
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

    return names.joined(separator: ",")
  }

  //
  // Unknown Events
  //

  public func appendUnknownEvent(_ entry: EventHistoryEntry) {
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
