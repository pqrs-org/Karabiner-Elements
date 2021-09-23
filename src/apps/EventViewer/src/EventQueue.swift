import SwiftUI

private func callback(_ deviceId: UInt64,
                      _ usagePage: Int32,
                      _ usage: Int32,
                      _ eventType: libkrbn_hid_value_event_type,
                      _ context: UnsafeMutableRawPointer?)
{
    let obj: EventQueue! = unsafeBitCast(context, to: EventQueue.self)

    DispatchQueue.main.async { [weak obj] in
        guard let obj = obj else { return }

        if !libkrbn_is_momentary_switch_event(usagePage, usage) {
            return
        }

        var buffer = [Int8](repeating: 0, count: 256)
        let entry = EventQueueEntry()

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
        // entry.name
        //

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

            if eventType == libkrbn_hid_value_event_type_key_down {
                obj.modifierFlags[deviceId]!.insert(modifierFlagName)
            } else {
                obj.modifierFlags[deviceId]!.remove(modifierFlagName)
            }
        }

        //
        // entry.eventType
        //

        switch eventType {
        case libkrbn_hid_value_event_type_key_down:
            entry.eventType = "down"
        case libkrbn_hid_value_event_type_key_up:
            entry.eventType = "up"
        case libkrbn_hid_value_event_type_single:
            break
        default:
            break
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
        // EventQueue.queue
        //

        obj.append(entry)

        //
        // simpleModificationJsonString
        //

        libkrbn_get_simple_modification_json_string(&buffer, buffer.count, usagePage, usage)
        let simpleModificationJsonString = String(cString: buffer)

        if simpleModificationJsonString != "" {
            obj.simpleModificationJsonString = simpleModificationJsonString

            libkrbn_get_momentary_switch_event_usage_name(&buffer, buffer.count, usagePage, usage)
            let usageName = String(cString: buffer)
            obj.addSimpleModificationButtonText = "Add \(usageName) to Karabiner-Elements"
        }
    }
}

public class EventQueueEntry: Identifiable, Equatable {
    public var id = UUID()
    public var eventType = ""
    public var usagePage = ""
    public var usage = ""
    public var name = ""
    public var misc = ""

    public static func == (lhs: EventQueueEntry, rhs: EventQueueEntry) -> Bool {
        lhs.id == rhs.id
    }
}

public class EventQueue: ObservableObject {
    public static let shared = EventQueue()

    let maxCount = 256
    var modifierFlags: [UInt64: Set<String>] = [:]

    @Published var queue: [EventQueueEntry] = []
    @Published var simpleModificationJsonString: String = ""
    @Published var addSimpleModificationButtonText: String = ""
    @Published var unknownEventEntries: [EventQueueEntry] = []

    init() {
        clear()

        let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
        libkrbn_enable_hid_value_monitor(callback, obj)
    }

    deinit {
        libkrbn_disable_hid_value_monitor()
    }

    public func observed() -> Bool {
        libkrbn_hid_value_monitor_observed()
    }

    public func append(_ entry: EventQueueEntry) {
        queue.append(entry)
        if queue.count > maxCount {
            queue.removeFirst()
        }
    }

    public func clear() {
        queue.removeAll()

        // Fill queue with empty entries to avoid SwiftUI List rendering corruption at MainView.swift.
        while queue.count < maxCount {
            queue.append(EventQueueEntry())
        }
    }

    public func copyToPasteboard() {
        var string = ""

        queue.forEach { entry in
            if entry.eventType.count > 0 {
                let eventType = "type:\(entry.eventType)".padding(toLength: 20, withPad: " ", startingAt: 0)
                let code = "HID usage: \(entry.usagePage),\(entry.usage)".padding(toLength: 20, withPad: " ", startingAt: 0)
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

    public func addSimpleModification() {
        guard let string = simpleModificationJsonString.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed) else { return }
        guard let url = URL(string: "karabiner://karabiner/simple_modifications/new?json=\(string)") else { return }
        NSWorkspace.shared.open(url)
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
}
