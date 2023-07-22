import Combine

class FingerStatusManager: ObservableObject {
  static let shared = FingerStatusManager()
  static let fingerCountChanged = Notification.Name("fingerCountChanged")

  private(set) var objectWillChange = ObservableObjectPublisher()
  private(set) var entries: [FingerStatusEntry] = []

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

      let e = getEntry(device: device, identifier: Int(finger.identifier))
      e.frame = Int(frame)
      e.point = NSMakePoint(
        CGFloat(finger.normalized.position.x),
        CGFloat(finger.normalized.position.y))

      // Note:
      // Once the point in targetArea, keep `ignored == NO`.
      if e.ignored {
        if NSPointInRect(e.point, targetArea) {
          e.ignored = false
        }
      }

      if e.touchedPhysically != touched {
        e.touchedPhysically = touched

        /*
        [self setFingerStatusEntryDelayTimer:e touched:touched];
        */
      }
    }

    //
    // Update physical untouched fingers
    //

    for e in entries {
      if e.device == device && e.frame != frame && e.touchedPhysically {
        e.touchedPhysically = false

        /*
        [self setFingerStatusEntryDelayTimer:e touched:NO];
        */
      }

      //print("\(e.touchedPhysically) \(e.point)")
    }

    //
    // Remove untouched fingers
    //

    entries.removeAll(where: { $0.touchedPhysically == false && $0.touchedFixed == false })

    //
    // Post notifications
    //

    NotificationCenter.default.post(name: FingerStatusManager.fingerCountChanged, object: nil)

    objectWillChange.send()
  }

  @MainActor
  var fingerCount: FingerCount {
    var fingerCount = FingerCount()

    for e in entries {
      if e.ignored {
        continue
      }

      if !e.touchedFixed {
        continue
      }

      if e.point.x < 0.5 {
        fingerCount.leftHalfAreaCount += 1
        if e.point.x < 0.25 {
          fingerCount.leftQuarterAreaCount += 1
        }
      } else {
        fingerCount.rightHalfAreaCount += 1
        if e.point.x > 0.75 {
          fingerCount.rightQuarterAreaCount += 1
        }
      }

      if e.point.y < 0.5 {
        fingerCount.lowerHalfAreaCount += 1
        if e.point.y < 0.25 {
          fingerCount.lowerQuarterAreaCount += 1
        }
      } else {
        fingerCount.upperHalfAreaCount += 1
        if e.point.y > 0.75 {
          fingerCount.upperQuarterAreaCount += 1
        }
      }

      fingerCount.totalCount += 1
    }

    return fingerCount
  }

  private func getEntry(device: MTDevice, identifier: Int) -> FingerStatusEntry {
    for e in entries {
      if e.device == device && e.identifier == identifier {
        return e
      }
    }

    let e = FingerStatusEntry(device: device, identifier: identifier)
    entries.append(e)
    return e
  }

}
