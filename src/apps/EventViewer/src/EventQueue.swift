import Cocoa

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
            entry.code = String(format: "0x%02x,0x%02x", usagePage, usage)
        } else {
            entry.code = String(format: "%d,%d", usagePage, usage)
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
            libkrbn_get_momentary_switch_event_usage_name(&buffer, buffer.count, usagePage, usage)
            let usageName = String(cString: buffer)

            obj.simpleModificationJsonString = simpleModificationJsonString
            obj.updateAddSimpleModificationButton("Add \(usageName) to Karabiner-Elements")
        }
    }
}

@objcMembers
public class EventQueueEntry: NSObject {
    var eventType: String = ""
    var code: String = ""
    var name: String = ""
    var misc: String = ""
}

public class EventQueue: NSObject, NSTableViewDataSource {
    var queue: [EventQueueEntry] = []
    let maxQueueCount = 256
    var modifierFlags: [UInt64: Set<String>] = [:]
    var simpleModificationJsonString: String = ""

    @IBOutlet var view: NSTableView!
    @IBOutlet var addSimpleModificationButton: NSButton!

    deinit {
        libkrbn_disable_hid_value_monitor()
    }

    public func setup() {
        updateAddSimpleModificationButton(nil)

        let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
        libkrbn_enable_hid_value_monitor(callback, obj)
    }

    public func observed() -> Bool {
        return libkrbn_hid_value_monitor_observed()
    }

    public func append(_ entry: EventQueueEntry) {
        queue.append(entry)
        if queue.count > maxQueueCount {
            queue.removeFirst()
        }
        refresh()
    }

    func refresh() {
        view.reloadData()
        view.scrollRowToVisible(queue.count - 1)
    }

    public func updateAddSimpleModificationButton(_ title: String?) {
        if title != nil {
            addSimpleModificationButton.title = title!
            addSimpleModificationButton.isHidden = false
        } else {
            addSimpleModificationButton.isHidden = true
        }
    }

    @IBAction func clear(_: NSButton) {
        queue.removeAll()
        updateAddSimpleModificationButton(nil)
        refresh()
    }

    @IBAction func copy(_: NSButton) {
        var string = ""

        queue.forEach { entry in
            let eventType = "type:\(entry.eventType)".padding(toLength: 20, withPad: " ", startingAt: 0)
            let code = "HID usage:\(entry.code)".padding(toLength: 20, withPad: " ", startingAt: 0)
            let name = "name:\(entry.name)".padding(toLength: 60, withPad: " ", startingAt: 0)
            let misc = "misc:\(entry.misc)"

            string.append("\(eventType) \(code) \(name) \(misc)\n")
        }

        if !string.isEmpty {
            let pboard = NSPasteboard.general
            pboard.clearContents()
            pboard.writeObjects([string as NSString])
        }
    }

    @IBAction func addSimpleModification(_: NSButton) {
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

    func pushMouseEvent(event: NSEvent, eventType: String) {
        let entry = EventQueueEntry()
        entry.eventType = eventType
        entry.code = String(event.buttonNumber)
        entry.name = "button\(event.buttonNumber + 1)"

        entry.misc = String(format: "{x:%d,y:%d} click_count:%d",
                            Int(event.locationInWindow.x),
                            Int(event.locationInWindow.y),
                            Int(event.clickCount))
        let flags = modifierFlagsString(event.modifierFlags)
        if !flags.isEmpty {
            entry.misc.append(" flags:\(flags)")
        }

        append(entry)
    }

    func pushScrollWheelEvent(event: NSEvent, eventType: String) {
        let entry = EventQueueEntry()
        entry.eventType = eventType
        entry.misc = String(format: "dx:%.03f dy:%.03f dz:%.03f",
                            event.deltaX,
                            event.deltaY,
                            event.deltaZ)

        append(entry)
    }

    func pushMouseEvent(_ event: NSEvent) {
        switch event.type {
        case .leftMouseDown,
             .rightMouseDown,
             .otherMouseDown,
             .leftMouseUp,
             .rightMouseUp,
             .otherMouseUp:
            // Do nothing
            break

        case .leftMouseDragged,
             .rightMouseDragged,
             .otherMouseDragged:
            pushMouseEvent(event: event, eventType: "mouse_dragged")

        case .scrollWheel:
            pushScrollWheelEvent(event: event, eventType: "scroll_wheel")

        default:
            // Do nothing
            break
        }
    }

    //
    // NSTableViewDataSource
    //

    public func numberOfRows(in _: NSTableView) -> Int {
        return queue.count
    }

    public func tableView(_: NSTableView,
                          objectValueFor tableColumn: NSTableColumn?,
                          row: Int) -> Any?
    {
        guard let identifier = tableColumn?.identifier else { return nil }
        return queue[row].value(forKey: identifier.rawValue)
    }
}
