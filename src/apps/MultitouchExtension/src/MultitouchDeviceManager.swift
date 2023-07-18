private func callback(
  _ device: MTDevice?,
  _ fingerData: UnsafeMutablePointer<Finger>?,
  _ fingerCount: Int32,
  _ timestamp: Double,
  _ frame: Int32
) -> Int32 {
  let fingers =
    fingerData != nil
    ? Array(UnsafeBufferPointer(start: fingerData, count: Int(fingerCount)))
    : []

  if let device = device {
    Task { @MainActor in
      FingerStatusManager.shared.update(
        device: device,
        fingers: fingers,
        timestamp: timestamp,
        frame: frame)
    }
  }

  return 0
}

class MultitouchDeviceManager {
  static let shared = MultitouchDeviceManager()

  private let notificationPort = IONotificationPortCreate(kIOMasterPortDefault)

  private var devices: [MTDevice] = []
  private var wakeObserver: NSObjectProtocol?

  @MainActor
  func setCallback(_ register: Bool) {
    print("setCallback \(register)")

    //
    // Unset callback first
    //

    for device in devices {
      MTDeviceStop(device, 0)
      MTUnregisterContactFrameCallback(device, callback)
    }

    devices.removeAll()

    //
    // Set callbacodk
    //

    if register {
      devices = (MTDeviceCreateList().takeUnretainedValue() as? [MTDevice]) ?? []

      for device in devices {
        MTRegisterContactFrameCallback(device, callback)
        MTDeviceStart(device, 0)
      }
    }
  }

  //
  // IONotification
  //

  private func releaseIterator(_ iterator: io_iterator_t) {
    while true {
      let obj = IOIteratorNext(iterator)
      if obj == 0 {
        break
      }

      IOObjectRelease(obj)
    }
  }

  @MainActor
  func registerIONotification() {
    print("registerIONotification")

    //
    // Relaunch if device is connected or disconnected
    //

    let match = IOServiceMatching("AppleMultitouchDevice") as NSMutableDictionary

    for notification in [
      kIOMatchedNotification,
      kIOTerminatedNotification,
    ] {
      do {
        var it: io_iterator_t = 0

        let kr = IOServiceAddMatchingNotification(
          notificationPort,
          notification,
          match,
          { _, _ in
            KarabinerKit.relaunch()
          },
          nil,
          &it)
        if kr != kIOReturnSuccess {
          print("IOServiceAddMatchingNotification error: \(kr)")
          return
        }

        releaseIterator(it)
      }
    }

    let loopSource = IONotificationPortGetRunLoopSource(notificationPort).takeUnretainedValue()
    CFRunLoopAddSource(RunLoop.current.getCFRunLoop(), loopSource, .defaultMode)
  }

  //
  // WakeNotification
  //

  func registerWakeNotification() {
    /*
  @weakify(self);

  NSNotificationCenter* center = [[NSWorkspace sharedWorkspace] notificationCenter];
  id o = [center addObserverForName:NSWorkspaceDidWakeNotification
                             object:nil
                              queue:[NSOperationQueue mainQueue]
                         usingBlock:^(NSNotification* note) {
                           @strongify(self);
                           if (!self) {
                             return;
                           }

                           NSLog(@"NSWorkspaceDidWakeNotification");

                           // sleep until devices are settled.
                           [NSThread sleepForTimeInterval:1.0];

                           if ([[NSUserDefaults standardUserDefaults] boolForKey:kRelaunchAfterWakeUpFromSleep]) {
                             double wait = [[[NSUserDefaults standardUserDefaults] stringForKey:kRelaunchWait] doubleValue];
                             if (wait > 0) {
                               [NSThread sleepForTimeInterval:wait];
                             }

                             [KarabinerKit relaunch];
                           }

                           [self setCallback:YES];
                         }];

  [self.observers addObserver:o notificationCenter:center];
  */
  }

  func unregisterWakeNotification() {
    if let o = wakeObserver {
      NotificationCenter.default.removeObserver(o)
      wakeObserver = nil
    }
  }
}
