import AsyncAlgorithms
import Combine

@MainActor
class FingerManager: ObservableObject {
  static let shared = FingerManager()

  // Multitouch events are triggered frequently as long as fingers remain on the device.
  // To avoid performance issues from updating a @Published variable on every event,
  // we update the raw values in real time but throttle updates to the @Published property.
  @Published var states: [FingerState] = []
  private var rawStates: [FingerState] = []

  @Published var fingerCount = FingerCount()

  private var notificationsTask: Task<Void, Never>?

  private let fingerStateChangedStream: AsyncStream<Void>
  private let fingerStateChangedContinuation: AsyncStream<Void>.Continuation
  private var fingerStateChangedTask: Task<Void, Never>?

  init() {
    var continuation: AsyncStream<Void>.Continuation!
    self.fingerStateChangedStream = AsyncStream<Void> { continuation = $0 }
    self.fingerStateChangedContinuation = continuation

    notificationsTask = Task {
      await withTaskGroup(of: Void.self) { group in
        group.addTask {
          for await _ in NotificationCenter.default.notifications(
            named: FingerState.fingerStateChanged
          ) {
            self.fingerStateChangedContinuation.yield(())
          }
        }
      }
    }

    self.fingerStateChangedTask = Task { @MainActor in
      // Since FingerState.fingerStateChanged notifications are sent continuously,
      // using debounce prevents objectWillChange from firing.
      // That's why _throttle needs to be used instead.
      for await _ in self.fingerStateChangedStream._throttle(for: .milliseconds(50)) {
        states = rawStates
        updateFingerCount()
      }
    }
  }

  func update(
    device: MTDevice,
    fingers: [Finger],
    timestamp: Double,
    frame: Int32
  ) {
    let targetArea = UserSettings.shared.targetArea
    let palmThreshold = UserSettings.shared.palmThreshold
    let now = Date()

    //
    // Update physical touched fingers
    //

    for finger in fingers {
      // state values:
      //   4: touched
      //   1-3,5-7: near
      let touched = (finger.state == 4)

      let s = getFingerState(device: device, identifier: Int(finger.identifier))
      s.frame = Int(frame)
      s.size = Double(finger.size)
      s.point = NSPoint(
        x: CGFloat(finger.normalized.position.x),
        y: CGFloat(finger.normalized.position.y))
      s.contactFrameArrivedAt = now

      // Note:
      // Once the point in targetArea, keep `ignored == NO`.
      if s.ignored {
        if targetArea.contains(s.point) {
          s.ignored = false
        }
      }

      // Note:
      // Once the finger is recognised as a palm, keep `palmed` in order to continue to recognise it as a palm even when the palm leaves.
      if Double(finger.size) > palmThreshold {
        s.palmed = true
      }

      if s.touchedPhysically != touched {
        s.touchedPhysically = touched

        s.setDelayTask(
          mode: touched
            ? FingerState.DelayMode.touched
            : FingerState.DelayMode.untouched)
      }
    }

    //
    // Update physical untouched fingers
    //

    for s in rawStates {
      if s.device == device && s.frame != frame && s.touchedPhysically {
        s.touchedPhysically = false

        s.setDelayTask(mode: FingerState.DelayMode.untouched)
      }

      // print("\(e.touchedPhysically) \(e.point)")
    }

    //
    // Remove untouched fingers
    //

    rawStates.removeAll(where: { $0.touchedPhysically == false && $0.touchedFixed == false })

    //
    // Remove FingerState which the contact frame has not been received for a certain period.
    //

    rawStates.removeAll(where: { now.timeIntervalSince($0.contactFrameArrivedAt) > 3.0 })

    //
    // Post notifications
    //

    NotificationCenter.default.post(name: FingerState.fingerStateChanged, object: nil)
  }

  private func updateFingerCount() {
    var c = FingerCount()
    let targetArea = UserSettings.shared.targetArea
    let x25 = targetArea.origin.x + targetArea.size.width * 0.25
    let x50 = targetArea.origin.x + targetArea.size.width * 0.5
    let x75 = targetArea.origin.x + targetArea.size.width * 0.75
    let y25 = targetArea.origin.y + targetArea.size.height * 0.25
    let y50 = targetArea.origin.y + targetArea.size.height * 0.5
    let y75 = targetArea.origin.y + targetArea.size.height * 0.75

    for s in rawStates {
      if s.ignored {
        continue
      }

      if !s.touchedFixed {
        continue
      }

      if s.palmed {
        // If palm is detected, we do not increment touched finger counter.

        if s.point.x < x50 {
          c.leftHalfAreaPalmCount += 1
        } else {
          c.rightHalfAreaPalmCount += 1
        }

        if s.point.y < y50 {
          c.lowerHalfAreaPalmCount += 1
        } else {
          c.upperHalfAreaPalmCount += 1
        }

        c.totalPalmCount += 1
      } else {
        if s.point.x < x50 {
          c.leftHalfAreaCount += 1
          if s.point.x < x25 {
            c.leftQuarterAreaCount += 1
          }
        } else {
          c.rightHalfAreaCount += 1
          if s.point.x > x75 {
            c.rightQuarterAreaCount += 1
          }
        }

        if s.point.y < y50 {
          c.lowerHalfAreaCount += 1
          if s.point.y < y25 {
            c.lowerQuarterAreaCount += 1
          }
        } else {
          c.upperHalfAreaCount += 1
          if s.point.y > y75 {
            c.upperQuarterAreaCount += 1
          }
        }

        c.totalCount += 1
      }
    }

    fingerCount = c
  }

  private func getFingerState(device: MTDevice, identifier: Int) -> FingerState {
    for s in rawStates {
      if s.device == device && s.identifier == identifier {
        return s
      }
    }

    let s = FingerState(device: device, identifier: identifier)
    rawStates.append(s)
    return s
  }
}
