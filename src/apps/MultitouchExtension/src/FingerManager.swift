import AsyncAlgorithms
import Combine

@MainActor
class FingerManager: ObservableObject {
  static let shared = FingerManager()

  // Multitouch events are triggered frequently as long as fingers remain on the device.
  // To avoid performance issues from updating a @Published variable on every event,
  // we update the raw values in real time but throttle updates to the @Published property.
  private var rawStates: [FingerState] = []
  // Tracking finger positions requires updating states at a high frame rate,
  // which is resource-intensive, so it should only be done when necessary.
  var trackStates = false
  @Published var states: [FingerState] = []

  @Published var fingerCount = FingerCount()

  private var timerTask: Task<Void, Never>?

  private func evaluateTimerState() {
    switch (rawStates.isEmpty, timerTask) {
    case (false, nil):
      startTimer()
    case (true, let task?):
      task.cancel()
      timerTask = nil
    default:
      break
    }
  }

  private func startTimer() {
    timerTask = Task { @MainActor in
      let timer = AsyncTimerSequence(interval: .milliseconds(20), clock: ContinuousClock())
      for await _ in timer {
        if Task.isCancelled { break }

        //
        // Update rawState
        //

        let now = Date()

        rawStates.forEach { $0.updateTouchedFixed(now: now) }

        // Remove untouched fingers
        rawStates.removeAll(where: { $0.touchedPhysically == false && $0.touchedFixed == false })

        // Remove FingerState which the contact frame has not been received for a certain period.
        rawStates.removeAll(where: { now.timeIntervalSince($0.contactFrameArrivedAt) > 3.0 })

        //
        // Update @Published variables
        //

        if trackStates {
          states = rawStates
        }

        updateFingerCount()

        //
        // Stop the timer when all fingers are lifted.
        //

        evaluateTimerState()
      }
    }
  }

  func update(
    mtDeviceRegistryEntryID: UInt64,
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
      let s = getFingerState(
        mtDeviceRegistryEntryID: mtDeviceRegistryEntryID,
        identifier: Int(finger.identifier))
      s.frame = Int(frame)
      s.size = Double(finger.size)
      s.point = NSPoint(
        x: CGFloat(finger.normalized.position.x),
        y: CGFloat(finger.normalized.position.y))
      s.contactFrameArrivedAt = now

      // state values:
      //   4: touched
      //   1-3,5-7: near
      s.touchedPhysically = (finger.state == 4)

      // Note:
      // Once the point in targetArea, keep `ignored == false`.
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
    }

    //
    // Update physical untouched fingers
    //

    for s in rawStates {
      if s.mtDeviceRegistryEntryID == mtDeviceRegistryEntryID
        && s.frame != frame
        && s.touchedPhysically
      {
        s.touchedPhysically = false
      }
      // print("\(e.touchedPhysically) \(e.point)")
    }

    evaluateTimerState()
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

    if fingerCount != c {
      fingerCount = c
    }
  }

  private func getFingerState(mtDeviceRegistryEntryID: UInt64, identifier: Int) -> FingerState {
    for s in rawStates {
      if s.mtDeviceRegistryEntryID == mtDeviceRegistryEntryID
        && s.identifier == identifier
      {
        return s
      }
    }

    let s = FingerState(
      mtDeviceRegistryEntryID: mtDeviceRegistryEntryID,
      identifier: identifier,
    )
    rawStates.append(s)
    return s
  }
}
