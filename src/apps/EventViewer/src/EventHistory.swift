import SwiftUI

private func callback(
  _ deviceId: UInt64,
  _ usagePage: Int32,
  _ usage: Int32,
  _ integerValue: Int64,
  _ context: UnsafeMutableRawPointer?
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

  // usage::generic_desktop::wheel
  if usagePage == 0x1, usage == 0x38 {
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

  let obj: EventHistory! = unsafeBitCast(context, to: EventHistory.self)

  DispatchQueue.main.async { [weak obj] in
    guard let obj = obj else { return }

    let entry = EventHistoryEntry()

    //
    // entry.code
    //

    if UserSettings.shared.showHex {
      entry.usagePage = String(format: "0x%02x", usagePage)
      entry.usage = String(format: "0x%02x", usage)
    } else {
      entry.usagePage = String(format: "%d", usagePage)
      entry.usage = String(format: "%d", usage)
    }

    //
    // Handle unknown events
    //

    if !libkrbn_is_momentary_switch_event_target(usagePage, usage) {
      entry.eventType = "\(integerValue)"
      obj.appendUnknownEvent(entry)
      return
    }

    //
    // entry.name
    //

    var buffer = [Int8](repeating: 0, count: 256)
    libkrbn_get_momentary_switch_event_json_string(&buffer, buffer.count, usagePage, usage)
    let jsonString = String(cString: buffer)

    entry.name = jsonString

    //
    // modifierFlags
    //

    if libkrbn_is_modifier_flag(usagePage, usage) {
      libkrbn_get_modifier_flag_name(&buffer, buffer.count, usagePage, usage)
      let modifierFlagName = String(cString: buffer)

      if obj.modifierFlags[deviceId] == nil {
        obj.modifierFlags[deviceId] = Set()
      }

      if integerValue != 0 {
        obj.modifierFlags[deviceId]!.insert(modifierFlagName)
      } else {
        obj.modifierFlags[deviceId]!.remove(modifierFlagName)
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

    if let set = obj.modifierFlags[deviceId] {
      if set.count > 0 {
        let flags = set.sorted().joined(separator: ",")
        entry.misc = "flags \(flags)"
      }
    }

    //
    // Add to entries
    //

    obj.append(entry)
  }
}

public class EventHistoryEntry: Identifiable, Equatable {
  public var id = UUID()
  public var eventType = ""
  public var usagePage = ""
  public var usage = ""
  public var name = ""
  public var misc = ""

  public static func == (lhs: EventHistoryEntry, rhs: EventHistoryEntry) -> Bool {
    lhs.id == rhs.id
  }
}

public class EventHistory: ObservableObject {
  public static let shared = EventHistory()

  // Keep maxCount small since too many entries causes performance issue at SwiftUI rendering.
  private let maxCount = 32
  public var modifierFlags: [UInt64: Set<String>] = [:]

  @Published var entries: [EventHistoryEntry] = []
  @Published var unknownEventEntries: [EventHistoryEntry] = []

  private init() {
    let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
    libkrbn_enable_hid_value_monitor(callback, obj)
  }

  deinit {
    libkrbn_disable_hid_value_monitor()
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
    var string = ""

    entries.forEach { entry in
      if entry.eventType.count > 0 {
        let eventType = "type:\(entry.eventType)".padding(toLength: 20, withPad: " ", startingAt: 0)
        let code = "HID usage: \(entry.usagePage),\(entry.usage)".padding(
          toLength: 20, withPad: " ", startingAt: 0)
        let name = "name:\(entry.name)".padding(toLength: 60, withPad: " ", startingAt: 0)
        let misc = "misc:\(entry.misc)"

        string.append("\(eventType) \(code) \(name) \(misc)\n")
      }
    }

    if !string.isEmpty {
      let pboard = NSPasteboard.general
      pboard.clearContents()
      pboard.writeObjects([string as NSString])
    }
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
    var string = ""

    unknownEventEntries.forEach { entry in
      if entry.eventType.count > 0 {
        let eventType = "value:\(entry.eventType)".padding(
          toLength: 20, withPad: " ", startingAt: 0)
        let code = "HID usage: \(entry.usagePage),\(entry.usage)".padding(
          toLength: 20, withPad: " ", startingAt: 0)

        string.append("\(eventType) \(code)\n")
      }
    }

    if !string.isEmpty {
      let pboard = NSPasteboard.general
      pboard.clearContents()
      pboard.writeObjects([string as NSString])
    }
  }
}
