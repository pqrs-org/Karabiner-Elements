// TODO: Remove @objc
@objc
class FingerStatusManager: NSObject {
  static let shared = FingerStatusManager()
  static let fingerStateChanged = Notification.Name("fingerStateChanged")

  private var entries: [FingerStatusEntry] = []

  /*
+ (instancetype)sharedFingerStatusManager {
  static dispatch_once_t once;
  static FingerStatusManager* manager;

  dispatch_once(&once, ^{
    manager = [FingerStatusManager new];
  });

  return manager;
}
*/

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
    }

    //
    // Post notifications
    //

    NotificationCenter.default.post(name: FingerStatusManager.fingerStateChanged, object: nil)
  }

  @MainActor
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

  /*
// Note: This method is called in @synchronized(self)
- (void)setFingerStatusEntryDelayTimer:(FingerStatusEntry*)entry
                               touched:(BOOL)touched {
  enum FingerStatusEntryTimerMode timerMode = FingerStatusEntryTimerModeNone;
  if (touched) {
    timerMode = FingerStatusEntryTimerModeTouched;
  } else {
    timerMode = FingerStatusEntryTimerModeUntouched;
  }

  if (entry.timerMode != timerMode) {
    entry.timerMode = timerMode;

    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];

    double delay = 0;
    if (touched) {
      delay = [defaults integerForKey:kDelayBeforeTurnOn];
    } else {
      delay = [defaults integerForKey:kDelayBeforeTurnOff];
    }

    [entry.delayTimer invalidate];

    @weakify(self);
    entry.delayTimer = [NSTimer timerWithTimeInterval:delay / 1000.0
                                              repeats:NO
                                                block:^(NSTimer* timer) {
                                                  @strongify(self);
                                                  if (!self) {
                                                    return;
                                                  }

                                                  @synchronized(self) {
                                                    entry.touchedFixed = touched;

                                                    if (!touched) {
                                                      [self.entries removeObjectIdenticalTo:entry];
                                                    }
                                                  }

                                                  [[NSNotificationCenter defaultCenter] postNotificationName:kFingerStateChanged
                                                                                                      object:self];
                                                }];
    [[NSRunLoop mainRunLoop] addTimer:entry.delayTimer forMode:NSRunLoopCommonModes];
  }
}

- (NSArray<FingerStatusEntry*>*)copyEntries {
  @synchronized(self) {
    return [[NSArray alloc] initWithArray:self.entries copyItems:YES];
  }
}

- (FingerCount)createFingerCount {
  @synchronized(self) {
    FingerCount fingerCount;
    memset(&fingerCount, 0, sizeof(fingerCount));

    for (FingerStatusEntry* e in self.entries) {
      if (e.ignored) {
        continue;
      }

      if (!e.touchedFixed) {
        continue;
      }

      if (e.point.x < 0.5) {
        ++fingerCount.leftHalfAreaCount;
        if (e.point.x < 0.25) {
          ++fingerCount.leftQuarterAreaCount;
        }
      } else {
        ++fingerCount.rightHalfAreaCount;
        if (e.point.x > 0.75) {
          ++fingerCount.rightQuarterAreaCount;
        }
      }

      if (e.point.y < 0.5) {
        ++fingerCount.lowerHalfAreaCount;
        if (e.point.y < 0.25) {
          ++fingerCount.lowerQuarterAreaCount;
        }
      } else {
        ++fingerCount.upperHalfAreaCount;
        if (e.point.y > 0.75) {
          ++fingerCount.upperQuarterAreaCount;
        }
      }

      ++fingerCount.totalCount;
    }

    return fingerCount;
  }
}
*/
}
