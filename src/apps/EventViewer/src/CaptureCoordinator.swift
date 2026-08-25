import SwiftUI
import os

struct CaptureEventToken: Equatable, Sendable {
  fileprivate let generation: UInt64
}

private enum CaptureEventTarget: Equatable, Sendable {
  case inactive
  case inputEvents
  case rawInputEvents(deviceId: UInt64)
  case rawInputRecords(deviceId: UInt64)
}

private struct CaptureEventValidatorState: Sendable {
  var generation: UInt64 = 0
  var target: CaptureEventTarget = .inactive
}

final class CaptureEventValidator: Sendable {
  static let shared = CaptureEventValidator()

  private let state = OSAllocatedUnfairLock(initialState: CaptureEventValidatorState())

  fileprivate func start(_ target: CaptureEventTarget) {
    state.withLock {
      $0.generation &+= 1
      $0.target = target
    }
  }

  func stop() {
    state.withLock {
      $0.generation &+= 1
      $0.target = .inactive
    }
  }

  func inputEventToken(deviceId: UInt64) -> CaptureEventToken? {
    state.withLock {
      switch $0.target {
      case .inputEvents:
        return CaptureEventToken(generation: $0.generation)
      case .rawInputEvents(let selectedDeviceId) where selectedDeviceId == deviceId:
        return CaptureEventToken(generation: $0.generation)
      case .inactive, .rawInputEvents, .rawInputRecords:
        return nil
      }
    }
  }

  func inputReportToken(deviceId: UInt64) -> CaptureEventToken? {
    state.withLock {
      guard case .rawInputRecords(let selectedDeviceId) = $0.target,
        selectedDeviceId == deviceId
      else {
        return nil
      }
      return CaptureEventToken(generation: $0.generation)
    }
  }

  func isCurrent(_ token: CaptureEventToken) -> Bool {
    state.withLock {
      $0.target != .inactive && $0.generation == token.generation
    }
  }
}

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
  private var emergencyStopMonitor: Any?

  @Published private(set) var currentMode: Mode?
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
    // SwiftUI may call the replacement view's onAppear before the old view's
    // onDisappear. Only the latest view is allowed to stop the capture.
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
      CaptureEventValidator.shared.start(.inputEvents)
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
      CaptureEventValidator.shared.start(
        .rawInputEvents(deviceId: rawInputEventsSelectedDeviceId))
      capturing = true
      startEmergencyStopMonitor()
      TemporarilyIgnoredDeviceManager.shared.activate(
        owner: .rawInputEvents,
        deviceId: rawInputEventsSelectedDeviceId)

    case .rawInputRecords:
      guard let rawInputRecordsSelectedDeviceId else {
        return
      }

      InputReportHistory.shared.clear()
      CaptureEventValidator.shared.start(
        .rawInputRecords(deviceId: rawInputRecordsSelectedDeviceId))
      capturing = true
      startEmergencyStopMonitor()
      TemporarilyIgnoredDeviceManager.shared.activate(
        owner: .rawInputRecords,
        deviceId: rawInputRecordsSelectedDeviceId)
      krbn_set_hid_input_report_capture_device(rawInputRecordsSelectedDeviceId)

    case nil:
      break
    }
  }

  func stopCapture() {
    CaptureEventValidator.shared.stop()

    switch currentMode {
    case .inputEvents:
      TemporarilyIgnoredDeviceManager.shared.deactivate(owner: .inputEvents)

    case .rawInputEvents:
      stopEmergencyStopMonitor()
      TemporarilyIgnoredDeviceManager.shared.deactivate(owner: .rawInputEvents)

    case .rawInputRecords:
      stopEmergencyStopMonitor()
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

  private func startEmergencyStopMonitor() {
    stopEmergencyStopMonitor()

    // Capturing raw input events or records temporarily disables Karabiner-Elements
    // modifications for the selected device, which may leave the keyboard unusable.
    // Keep Escape available as an emergency way to stop the capture. Only Escape is
    // consumed here; all other key-down events continue through the application.
    emergencyStopMonitor = NSEvent.addLocalMonitorForEvents(
      matching: .keyDown
    ) { (event: NSEvent) -> NSEvent? in
      if event.keyCode == 53 {  // Escape
        Task { @MainActor in
          CaptureCoordinator.shared.stopCapture()
        }
        return nil
      }

      return event
    }
  }

  private func stopEmergencyStopMonitor() {
    if let emergencyStopMonitor {
      NSEvent.removeMonitor(emergencyStopMonitor)
      self.emergencyStopMonitor = nil
    }
  }
}
