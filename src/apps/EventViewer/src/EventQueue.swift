import AnyCodable
import Cocoa

private class SimpleModificationJson: Codable {
  var from: [String: AnyCodable] = [:]
}

private func callback(_ deviceId: UInt64,
                      _ type: libkrbn_hid_value_type,
                      _ value: UInt32,
                      _ eventType: libkrbn_hid_value_event_type,
                      _ context: UnsafeMutableRawPointer?)
{
  let obj: EventQueue! = unsafeBitCast(context, to: EventQueue.self)

  DispatchQueue.main.async { [weak obj] in
    guard let obj = obj else { return }

    let entry = EventQueueEntry()
    let simpleModificationJson = SimpleModificationJson()

    //
    // entry.code
    //

    if UserDefaults.standard.bool(forKey: "kShowHex") {
      entry.code = String(format: "0x%02x", value)
    } else {
      entry.code = String(value)
    }

    //
    // entry.name
    //

    var keyType = ""
    var buffer = [Int8](repeating: 0, count: 256)

    switch type {
    case libkrbn_hid_value_type_key_code:
      keyType = "key"
      libkrbn_get_key_code_name(&buffer, buffer.count, value)
      entry.name = String(cString: buffer)

      //
      // modifierFlags
      //

      if libkrbn_is_modifier_flag(value) {
        if obj.modifierFlags[deviceId] == nil {
          obj.modifierFlags[deviceId] = Set()
        }

        if eventType == libkrbn_hid_value_event_type_key_down {
          obj.modifierFlags[deviceId]!.insert(entry.name)
        } else {
          obj.modifierFlags[deviceId]!.remove(entry.name)
        }
      }

      //
      // simpleModificationJson
      //

      do {
        var unnamedNumber: UInt32 = 0
        if libkrbn_find_unnamed_key_code_number(&unnamedNumber, &buffer) {
          simpleModificationJson.from["key_code"] = AnyCodable(unnamedNumber)
        } else {
          simpleModificationJson.from["key_code"] = AnyCodable(entry.name)
        }
      }

    case libkrbn_hid_value_type_consumer_key_code:
      keyType = "consumer_key"
      libkrbn_get_consumer_key_code_name(&buffer, buffer.count, value)
      entry.name = String(cString: buffer)

      //
      // simpleModificationJson
      //

      do {
        var unnamedNumber: UInt32 = 0
        if libkrbn_find_unnamed_consumer_key_code_number(&unnamedNumber, &buffer) {
          simpleModificationJson.from["consumer_key_code"] = AnyCodable(unnamedNumber)
        } else {
          simpleModificationJson.from["consumer_key_code"] = AnyCodable(entry.name)
        }
      }

    default:
      break
    }

    //
    // entry.eventType
    //

    switch eventType {
    case libkrbn_hid_value_event_type_key_down:
      entry.eventType = "\(keyType)_down"
    case libkrbn_hid_value_event_type_key_up:
      entry.eventType = "\(keyType)_up"
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

    if simpleModificationJson.from.count > 0 {
      let jsonEncoder = JSONEncoder()
      if let data = try? jsonEncoder.encode(simpleModificationJson) {
        if let jsonString = String(data: data, encoding: .utf8) {
          obj.simpleModificationJsonString = jsonString
          obj.updateAddSimpleModificationButton("Add `\(entry.name)` to Karabiner-Elements")
        }
      }
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

@objc
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

  @objc
  public func setup() {
    updateAddSimpleModificationButton(nil)

    let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
    libkrbn_enable_hid_value_monitor(callback, obj)
  }

  @objc
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
      let code = "code:\(entry.code)".padding(toLength: 15, withPad: " ", startingAt: 0)
      let name = "name:\(entry.name)".padding(toLength: 20, withPad: " ", startingAt: 0)
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
    var flags = modifierFlagsString(event.modifierFlags)
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

  @objc
  func pushMouseEvent(_ event: NSEvent) {
    switch event.type {
    case .leftMouseDown,
         .rightMouseDown,
         .otherMouseDown:
      pushMouseEvent(event: event, eventType: "button_down")

    case .leftMouseUp,
         .rightMouseUp,
         .otherMouseUp:
      pushMouseEvent(event: event, eventType: "button_up")

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
