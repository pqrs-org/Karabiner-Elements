import SwiftUI

@MainActor
final class CaptureCoordinator: ObservableObject {
  enum Mode: Equatable {
    case inputEvents
    case rawInputEvents
    case rawInputRecords
  }

  struct Session: Equatable {
    fileprivate let generation: UInt64
  }

  static let shared = CaptureCoordinator()

  private var generation: UInt64 = 0
  private var currentSession: Session?
  private var currentMode: Mode?
  private var keyDownMonitor: Any?

  @Published private(set) var capturing = false
  @Published private(set) var rawInputEventsSelectedDeviceId: UInt64?
  @Published private(set) var rawInputRecordsSelectedDeviceId: UInt64?

  func begin(_ mode: Mode) -> Session {
    stopCurrentSession()

    generation &+= 1
    let session = Session(generation: generation)
    currentSession = session
    currentMode = mode

    let eventHistory = EventHistory.shared

    switch mode {
    case .inputEvents:
      eventHistory.startPointingButtonModifierFlagsMonitoring()
      startCapture()

    case .rawInputEvents:
      eventHistory.startPointingButtonModifierFlagsMonitoring()
      eventHistory.clear()

    case .rawInputRecords:
      break
    }

    return session
  }

  func end(_ session: Session) {
    // SwiftUI may start the replacement view's task before running the old
    // task's defer. Only the latest view is allowed to stop the capture.
    guard currentSession == session else {
      return
    }

    stopCurrentSession()
  }

  func startCapture() {
    guard !capturing else {
      return
    }

    switch currentMode {
    case .inputEvents:
      EventHistory.shared.clear()
      EventHistory.shared.resetModifierFlags()
      capturing = true
      TemporarilyIgnoredDeviceManager.shared.activate(
        owner: .inputEvents,
        deviceId: nil)

    case .rawInputEvents:
      guard let rawInputEventsSelectedDeviceId else {
        return
      }

      EventHistory.shared.clear()
      EventHistory.shared.resetModifierFlags()
      capturing = true
      startKeyDownMonitor()
      TemporarilyIgnoredDeviceManager.shared.activate(
        owner: .rawInputEvents,
        deviceId: rawInputEventsSelectedDeviceId)

    case .rawInputRecords:
      guard let rawInputRecordsSelectedDeviceId else {
        return
      }

      InputReportHistory.shared.clear()
      capturing = true
      startKeyDownMonitor()
      TemporarilyIgnoredDeviceManager.shared.activate(
        owner: .rawInputRecords,
        deviceId: rawInputRecordsSelectedDeviceId)
      krbn_set_hid_input_report_capture_device(rawInputRecordsSelectedDeviceId)

    case nil:
      break
    }
  }

  func stopCapture() {
    switch currentMode {
    case .inputEvents:
      TemporarilyIgnoredDeviceManager.shared.deactivate(owner: .inputEvents)

    case .rawInputEvents:
      stopKeyDownMonitor()
      TemporarilyIgnoredDeviceManager.shared.deactivate(owner: .rawInputEvents)

    case .rawInputRecords:
      stopKeyDownMonitor()
      krbn_set_hid_input_report_capture_device(0)
      TemporarilyIgnoredDeviceManager.shared.deactivate(owner: .rawInputRecords)

    case nil:
      break
    }

    capturing = false
  }

  func selectRawInputEventsDevice(_ deviceId: UInt64?) {
    guard deviceId != rawInputEventsSelectedDeviceId else {
      return
    }

    if currentMode == .rawInputEvents {
      stopCapture()
    }
    rawInputEventsSelectedDeviceId = deviceId
    EventHistory.shared.clear()
    EventHistory.shared.resetModifierFlags()
  }

  func selectRawInputRecordsDevice(_ deviceId: UInt64?) {
    guard deviceId != rawInputRecordsSelectedDeviceId else {
      return
    }

    if currentMode == .rawInputRecords {
      stopCapture()
    }
    rawInputRecordsSelectedDeviceId = deviceId
    InputReportHistory.shared.clear()
  }

  func rawInputEventsDeviceDisconnected() {
    selectRawInputEventsDevice(nil)
  }

  func rawInputRecordsDeviceDisconnected() {
    selectRawInputRecordsDevice(nil)
  }

  func acceptsInputEvent(deviceId: UInt64) -> Bool {
    guard capturing else {
      return false
    }

    switch currentMode {
    case .inputEvents:
      return true
    case .rawInputEvents:
      return deviceId == rawInputEventsSelectedDeviceId
    case .rawInputRecords, nil:
      return false
    }
  }

  func acceptsInputReport(deviceId: UInt64) -> Bool {
    capturing &&
      currentMode == .rawInputRecords &&
      deviceId == rawInputRecordsSelectedDeviceId
  }

  func coreServiceDisconnected() {
    switch currentMode {
    case .rawInputEvents, .rawInputRecords:
      stopCapture()
    case .inputEvents, nil:
      break
    }
  }

  private func stopCurrentSession() {
    guard let currentMode else {
      return
    }

    stopCapture()

    switch currentMode {
    case .inputEvents, .rawInputEvents:
      EventHistory.shared.stopPointingButtonModifierFlagsMonitoring()

    case .rawInputRecords:
      break
    }

    currentSession = nil
    self.currentMode = nil
  }

  private func startKeyDownMonitor() {
    stopKeyDownMonitor()

    keyDownMonitor = NSEvent.addLocalMonitorForEvents(matching: .keyDown) {
      (event: NSEvent) -> NSEvent? in
      if event.keyCode == 53 {  // Escape
        Task { @MainActor in
          CaptureCoordinator.shared.stopCapture()
        }
        return nil
      }

      return event
    }
  }

  private func stopKeyDownMonitor() {
    if let keyDownMonitor {
      NSEvent.removeMonitor(keyDownMonitor)
      self.keyDownMonitor = nil
    }
  }
}
