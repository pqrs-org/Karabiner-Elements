#import "MultitouchDeviceManager.h"
#import "FingerStatusManager.h"
#import "KarabinerKit/KarabinerKit.h"
#import <pqrs/weakify.h>

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

@interface MultitouchDeviceManager ()

@property(copy) NSArray* devices;
@property IONotificationPortRef notificationPort;
@property CFRunLoopSourceRef loopSource;
@property KarabinerKitSmartObserverContainer* observers;

@end

@implementation MultitouchDeviceManager

- (instancetype)init {
  self = [super init];

  if (self) {
    _devices = [NSMutableArray new];
    _observers = [KarabinerKitSmartObserverContainer new];
  }

  return self;
}

+ (instancetype)sharedMultitouchDeviceManager {
  static dispatch_once_t once;
  static MultitouchDeviceManager* manager;

  dispatch_once(&once, ^{
    manager = [MultitouchDeviceManager new];
  });

  return manager;
}

- (void)setCallback:(BOOL)set {
  @synchronized(self) {
    //
    // Unset callback (even if set is YES.)
    //

    if (self.devices) {
      for (NSUInteger i = 0; i < [self.devices count]; ++i) {
        MTDeviceRef device = (__bridge MTDeviceRef)(self.devices[i]);
        if (!device) continue;

        MTDeviceStop(device, 0);
        MTUnregisterContactFrameCallback(device, callback);
      }

      self.devices = [NSArray new];
    }

    //
    // Set callback if needed
    //

    if (set) {
      self.devices = (NSArray*)CFBridgingRelease(MTDeviceCreateList());
      if (self.devices) {
        for (NSUInteger i = 0; i < [self.devices count]; ++i) {
          MTDeviceRef device = (__bridge MTDeviceRef)(self.devices[i]);
          if (!device) continue;

          MTRegisterContactFrameCallback(device, callback);
          MTDeviceStart(device, 0);
        }
      }
    }
  }
}

//
// IONotification
//

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

- (void)registerIONotification {
  NSLog(@"registerIONotification");

  @synchronized(self) {
    if (self.notificationPort) {
      [self unregisterIONotification];
    }

    self.notificationPort = IONotificationPortCreate(kIOMasterPortDefault);
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
}

- (void)unregisterIONotification {
  NSLog(@"unregisterIONotification");

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
}

//
// WakeNotification
//

- (void)registerWakeNotification {
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

                           if ([[NSUserDefaults standardUserDefaults] boolForKey:@"relaunchAfterWakeUpFromSleep"]) {
                             double wait = [[[NSUserDefaults standardUserDefaults] stringForKey:@"relaunchWait"] doubleValue];
                             if (wait > 0) {
                               [NSThread sleepForTimeInterval:wait];
                             }

                             [KarabinerKit relaunch];
                           }

                           [self setCallback:YES];
                         }];

  [self.observers addObserver:o notificationCenter:center];
}

- (void)unregisterWakeNotification {
  self.observers = [KarabinerKitSmartObserverContainer new];
}

@end
