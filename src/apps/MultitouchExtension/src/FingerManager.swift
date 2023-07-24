import Combine

class FingerManager: ObservableObject {
  static let shared = FingerManager()

  private(set) var objectWillChange = ObservableObjectPublisher()
  private(set) var states: [FingerState] = []
  // Task to reduce the frequency of calling objectWillChange.send
  private var objectWillChangeTask: Task<(), Never>?

  init() {
    NotificationCenter.default.addObserver(
      forName: FingerState.fingerStateChanged,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      guard let self = self else { return }

      if self.objectWillChangeTask == nil {
        self.objectWillChangeTask = Task { @MainActor in
          do {
            try await Task.sleep(nanoseconds: 50 * NSEC_PER_MSEC)
          } catch {
            print(error.localizedDescription)
          }

          self.objectWillChange.send()

          self.objectWillChangeTask = nil
        }
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
