import AppKit
import Combine
import CoreGraphics
import Dispatch

@MainActor
final class MacOSKeyEventCapture: ObservableObject {
  struct Entry: Identifiable {
    enum Interruption: String {
      case tapDisabledByTimeout
      case tapDisabledByUserInput
      case secureKeyboardEntryEnabled
    }

    enum Payload {
      case keyEvent(type: String, keyCode: Int64, flags: String, isRepeat: Bool)
      case interruption(Interruption)
    }

    let id: UInt64
    let timestamp: UInt64
    let payload: Payload
  }

  static let shared = MacOSKeyEventCapture()

  @Published private(set) var entries: [Entry] = []
  @Published private(set) var failedToStart = false

  private var tap: CFMachPort?
  private var source: CFRunLoopSource?
  private var nextID: UInt64 = 0
  private static let historyLimit = 1000
  private static let modifierFlags: [(CGEventFlags, String)] = [
    (.maskAlphaShift, "caps_lock"), (.maskShift, "shift"), (.maskControl, "control"),
    (.maskAlternate, "option"), (.maskCommand, "command"), (.maskSecondaryFn, "fn"),
  ]

  private init() {}

  func start() -> Bool {
    guard tap == nil else { return true }
    failedToStart = false

    guard CGPreflightListenEventAccess() || CGRequestListenEventAccess() else {
      InputMonitoringAlertData.shared.showing = true
      return false
    }

    let mask = [CGEventType.keyDown, .keyUp, .flagsChanged].reduce(CGEventMask(0)) {
      $0 | (1 << $1.rawValue)
    }
    let eventTap = CGEvent.tapCreate(
      tap: .cgSessionEventTap,
      place: .headInsertEventTap,
      options: .listenOnly,
      eventsOfInterest: mask,
      callback: { _, type, event, _ in
        // The tap's run-loop source is installed on the main thread.
        switch type {
        case .tapDisabledByTimeout, .tapDisabledByUserInput:
          MainActor.assumeIsolated {
            MacOSKeyEventCapture.shared.resumeAfterInterruption(
              type == .tapDisabledByTimeout ? .tapDisabledByTimeout : .tapDisabledByUserInput)
          }
          return nil
        default:
          MainActor.assumeIsolated {
            MacOSKeyEventCapture.shared.receive(type: type, event: event)
          }
          return Unmanaged.passUnretained(event)
        }
      },
      userInfo: nil
    )

    guard let tap = eventTap,
      let source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0)
    else {
      if let eventTap {
        CFMachPortInvalidate(eventTap)
      }
      failedToStart = true
      return false
    }

    clear()
    self.tap = tap
    self.source = source
    CFRunLoopAddSource(CFRunLoopGetMain(), source, .commonModes)
    CGEvent.tapEnable(tap: tap, enable: true)
    return true
  }

  func stop() {
    if let tap {
      CGEvent.tapEnable(tap: tap, enable: false)
      CFMachPortInvalidate(tap)
    }
    if let source {
      CFRunLoopRemoveSource(CFRunLoopGetMain(), source, .commonModes)
    }
    tap = nil
    source = nil
    failedToStart = false
  }

  func clear() {
    entries.removeAll()
  }

  func copyToPasteboardJSON() {
    let objects = entries.map { entry -> [String: Any] in
      switch entry.payload {
      case .keyEvent(let type, let keyCode, let flags, let isRepeat):
        return [
          "timestamp_ns": entry.timestamp,
          "type": type,
          "key_code": keyCode,
          "flags": flags,
          "repeat": isRepeat,
        ]
      case .interruption(let reason):
        return ["timestamp_ns": entry.timestamp, "type": reason.rawValue]
      }
    }
    guard
      let data = try? JSONSerialization.data(
        withJSONObject: objects, options: [.prettyPrinted, .sortedKeys]),
      let text = String(data: data, encoding: .utf8)
    else { return }

    copyToPasteboard(text + "\n")
  }

  func copyToPasteboardTSV() {
    let header = "timestamp_ns\ttype\tkey_code\tflags\trepeat"
    let rows = entries.map { entry in
      switch entry.payload {
      case .keyEvent(let type, let keyCode, let flags, let isRepeat):
        return "\(entry.timestamp)\t\(type)\t\(keyCode)\t\(flags)\t\(isRepeat ? 1 : 0)"
      case .interruption(let reason):
        return "\(entry.timestamp)\t\(reason.rawValue)\t\t\t"
      }
    }
    copyToPasteboard(([header] + rows).joined(separator: "\n") + "\n")
  }

  private func copyToPasteboard(_ text: String) {
    let pasteboard = NSPasteboard.general
    pasteboard.clearContents()
    pasteboard.writeObjects([text as NSString])
  }

  func recordInterruption(_ reason: Entry.Interruption) {
    guard tap != nil else { return }

    // The notification marks when the interruption was detected, not when events stopped.
    append(timestamp: DispatchTime.now().uptimeNanoseconds, payload: .interruption(reason))
  }

  private func resumeAfterInterruption(_ reason: Entry.Interruption) {
    recordInterruption(reason)
    if let tap {
      CGEvent.tapEnable(tap: tap, enable: true)
    }
  }

  private func receive(type: CGEventType, event: CGEvent) {
    guard tap != nil else { return }

    let name: String
    switch type {
    case .keyDown: name = "keyDown"
    case .keyUp: name = "keyUp"
    case .flagsChanged: name = "flagsChanged"
    default:
      return
    }

    let flags = event.flags
    let flagNames = Self.modifierFlags.compactMap { flag, name in
      flags.contains(flag) ? name : nil
    }
    let rawFlags = "0x" + String(flags.rawValue, radix: 16)
    append(
      timestamp: event.timestamp,
      payload: .keyEvent(
        type: name,
        keyCode: event.getIntegerValueField(.keyboardEventKeycode),
        flags: ([rawFlags] + flagNames).joined(separator: " "),
        isRepeat: type == .keyDown && event.getIntegerValueField(.keyboardEventAutorepeat) != 0))
  }

  private func append(timestamp: UInt64, payload: Entry.Payload) {
    let entry = Entry(id: nextID, timestamp: timestamp, payload: payload)
    nextID &+= 1
    if entries.count == Self.historyLimit {
      entries.removeFirst()
    }
    entries.append(entry)
  }
}
