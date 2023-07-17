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

  private var devices: [MTDeviceRef] = []
  private let notificationPort = IONotificationPortCreate(kIOMasterPortDefault)
  private var loopSource: Unmanaged<CFRunLoopSource>?
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

  /*
- (void)releaseIterator:(io_iterator_t)iterator {
  for (;;) {
    io_object_t obj = IOIteratorNext(iterator);
    if (!obj) break;

    IOObjectRelease(obj);
  }
}

static void relaunch(void* refcon, io_iterator_t iterator) {
  // Relaunch when devices are plugged/unplugged.
  [KarabinerKit relaunch];
}
*/

  func registerIONotification() {
    print("registerIONotification")

    /*

  @synchronized(self) {
    if (self.notificationPort) {
      [self unregisterIONotification];
    }

    self.notificationPort = IONotificationPortCreate(0);
    if (!self.notificationPort) {
      NSLog(@"[ERROR] IONotificationPortCreate");
      return;
    }

    {
      // ------------------------------------------------------------
      NSMutableDictionary* match = (__bridge NSMutableDictionary*)(IOServiceMatching("AppleMultitouchDevice"));

      // ----------------------------------------------------------------------
      io_iterator_t it;
      kern_return_t kr;

      // for kIOTerminatedNotification
      kr = IOServiceAddMatchingNotification(self.notificationPort,
                                            kIOTerminatedNotification,
                                            (__bridge CFMutableDictionaryRef)(match),
                                            relaunch,
                                            (__bridge void*)(self),
                                            &it);
      if (kr != kIOReturnSuccess) {
        NSLog(@"[ERROR] IOServiceAddMatchingNotification");
        return;
      }
      [self releaseIterator:it];
    }

    {
      // ------------------------------------------------------------
      NSMutableDictionary* match = (__bridge NSMutableDictionary*)(IOServiceMatching("AppleMultitouchDevice"));

      // ----------------------------------------------------------------------
      io_iterator_t it;
      kern_return_t kr;

      // for kIOMatchedNotification
      kr = IOServiceAddMatchingNotification(self.notificationPort,
                                            kIOMatchedNotification,
                                            (__bridge CFMutableDictionaryRef)(match),
                                            relaunch,
                                            (__bridge void*)(self),
                                            &it);
      if (kr != kIOReturnSuccess) {
        NSLog(@"[ERROR] IOServiceAddMatchingNotification");
        return;
      }
      [self releaseIterator:it];
    }

    // ----------------------------------------------------------------------
    self.loopSource = IONotificationPortGetRunLoopSource(self.notificationPort);
    if (!self.loopSource) {
      NSLog(@"[ERROR] IONotificationPortGetRunLoopSource");
      return;
    }
    CFRunLoopAddSource(CFRunLoopGetCurrent(), self.loopSource, kCFRunLoopDefaultMode);
  }
  */
  }

  func unregisterIONotification() {
    print("unregisterIONotification")

    /*
  @synchronized(self) {
    if (self.notificationPort) {
      if (self.loopSource) {
        CFRunLoopSourceInvalidate(self.loopSource);
        self.loopSource = nil;
      }
      IONotificationPortDestroy(self.notificationPort);
      self.notificationPort = nil;
    }
  }
  */
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
