import AsyncAlgorithms
import Combine

class FingerManager: ObservableObject {
  static let shared = FingerManager()

  private(set) var objectWillChange = ObservableObjectPublisher()
  private(set) var states: [FingerState] = []

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
        self.objectWillChange.send()
      }
    }
  }

  @MainActor
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

    for s in states {
      if s.device == device && s.frame != frame && s.touchedPhysically {
        s.touchedPhysically = false

        s.setDelayTask(mode: FingerState.DelayMode.untouched)
      }

      // print("\(e.touchedPhysically) \(e.point)")
    }

    //
    // Remove untouched fingers
    //

    states.removeAll(where: { $0.touchedPhysically == false && $0.touchedFixed == false })

    //
    // Remove FingerState which the contact frame has not been received for a certain period.
    //

    states.removeAll(where: { now.timeIntervalSince($0.contactFrameArrivedAt) > 3.0 })

    //
    // Post notifications
    //

    NotificationCenter.default.post(name: FingerState.fingerStateChanged, object: nil)
  }

  @MainActor
  var fingerCount: FingerCount {
    var fingerCount = FingerCount()
    let targetArea = UserSettings.shared.targetArea
    let x25 = targetArea.origin.x + targetArea.size.width * 0.25
    let x50 = targetArea.origin.x + targetArea.size.width * 0.5
    let x75 = targetArea.origin.x + targetArea.size.width * 0.75
    let y25 = targetArea.origin.y + targetArea.size.height * 0.25
    let y50 = targetArea.origin.y + targetArea.size.height * 0.5
    let y75 = targetArea.origin.y + targetArea.size.height * 0.75

    for s in states {
      if s.ignored {
        continue
      }

      if !s.touchedFixed {
        continue
      }

      if s.palmed {
        // If palm is detected, we do not increment touched finger counter.

        if s.point.x < x50 {
          fingerCount.leftHalfAreaPalmCount += 1
        } else {
          fingerCount.rightHalfAreaPalmCount += 1
        }

        if s.point.y < y50 {
          fingerCount.lowerHalfAreaPalmCount += 1
        } else {
          fingerCount.upperHalfAreaPalmCount += 1
        }

        fingerCount.totalPalmCount += 1
      } else {
        if s.point.x < x50 {
          fingerCount.leftHalfAreaCount += 1
          if s.point.x < x25 {
            fingerCount.leftQuarterAreaCount += 1
          }
        } else {
          fingerCount.rightHalfAreaCount += 1
          if s.point.x > x75 {
            fingerCount.rightQuarterAreaCount += 1
          }
        }

        if s.point.y < y50 {
          fingerCount.lowerHalfAreaCount += 1
          if s.point.y < y25 {
            fingerCount.lowerQuarterAreaCount += 1
          }
        } else {
          fingerCount.upperHalfAreaCount += 1
          if s.point.y > y75 {
            fingerCount.upperQuarterAreaCount += 1
          }
        }

        fingerCount.totalCount += 1
      }
    }

    return fingerCount
  }

  private func getFingerState(device: MTDevice, identifier: Int) -> FingerState {
    for s in states {
      if s.device == device && s.identifier == identifier {
        return s
      }
    }

    let s = FingerState(device: device, identifier: identifier)
    states.append(s)
    return s
  }
}
