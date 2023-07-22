import Combine

class FingerManager: ObservableObject {
  static let shared = FingerManager()

  private(set) var objectWillChange = ObservableObjectPublisher()
  private(set) var states: [FingerState] = []

  init() {
    NotificationCenter.default.addObserver(
      forName: FingerState.fingerStateChanged,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      guard let self = self else { return }

      self.objectWillChange.send()
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
      s.point = NSMakePoint(
        CGFloat(finger.normalized.position.x),
        CGFloat(finger.normalized.position.y))

      // Note:
      // Once the point in targetArea, keep `ignored == NO`.
      if s.ignored {
        if NSPointInRect(s.point, targetArea) {
          s.ignored = false
        }
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

      //print("\(e.touchedPhysically) \(e.point)")
    }

    //
    // Remove untouched fingers
    //

    states.removeAll(where: { $0.touchedPhysically == false && $0.touchedFixed == false })

    //
    // Post notifications
    //

    NotificationCenter.default.post(name: FingerState.fingerStateChanged, object: nil)
  }

  @MainActor
  var fingerCount: FingerCount {
    var fingerCount = FingerCount()

    for s in states {
      if s.ignored {
        continue
      }

      if !s.touchedFixed {
        continue
      }

      if s.point.x < 0.5 {
        fingerCount.leftHalfAreaCount += 1
        if s.point.x < 0.25 {
          fingerCount.leftQuarterAreaCount += 1
        }
      } else {
        fingerCount.rightHalfAreaCount += 1
        if s.point.x > 0.75 {
          fingerCount.rightQuarterAreaCount += 1
        }
      }

      if s.point.y < 0.5 {
        fingerCount.lowerHalfAreaCount += 1
        if s.point.y < 0.25 {
          fingerCount.lowerQuarterAreaCount += 1
        }
      } else {
        fingerCount.upperHalfAreaCount += 1
        if s.point.y > 0.75 {
          fingerCount.upperQuarterAreaCount += 1
        }
      }

      fingerCount.totalCount += 1
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
