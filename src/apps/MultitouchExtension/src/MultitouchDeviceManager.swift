/*
//
// Multitouch callback
//

static int callback(MTDeviceRef device,
                    Finger* data,
                    int fingers,
                    double timestamp,
                    int frame) {
  if (!data) {
    fingers = 0;
  }

  if (device) {
    FingerStatusManager* manager = [FingerStatusManager sharedFingerStatusManager];
    [manager update:device
               data:data
            fingers:fingers
          timestamp:timestamp
              frame:frame];
  }

  return 0;
}
*/

// TODO: Remove @objc
class MultitouchDeviceManager {
  static let shared = MultitouchDeviceManager()

  private let notificationPort = IONotificationPortCreate(kIOMasterPortDefault)

  private var devices: [MTDeviceRef] = []
  private var wakeObserver: NSObjectProtocol?

  func setCallback(_ set: Bool) {
    /*
  @synchronized(self) {
    //
    // Unset callback (even if set is YES.)
    //

    if (self.devices) {
      for (NSUInteger i = 0; i < self.devices.count; ++i) {
        MTDeviceRef device = (__bridge MTDeviceRef)(self.devices[i]);
        if (!device) continue;

        MTDeviceStop(device, 0);
        MTUnregisterContactFrameCallback(device, callback);
      }

      self.devices = nil;
    }

    //
    // Set callback if needed
    //

    if (set) {
      self.devices = (NSArray*)CFBridgingRelease(MTDeviceCreateList());
      if (self.devices) {
        for (NSUInteger i = 0; i < self.devices.count; ++i) {
          MTDeviceRef device = (__bridge MTDeviceRef)(self.devices[i]);
          if (!device) continue;

          MTRegisterContactFrameCallback(device, callback);
          MTDeviceStart(device, 0);
        }
      }
    }
  }
  */
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

    let runLoop = CFRunLoopGetCurrent()
    let loopSource = IONotificationPortGetRunLoopSource(notificationPort).takeUnretainedValue()
    CFRunLoopAddSource(runLoop, loopSource, .defaultMode)
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
