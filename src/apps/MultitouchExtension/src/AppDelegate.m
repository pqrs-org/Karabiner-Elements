@import IOKit;
#import "AppDelegate.h"
#import "FingerStatusManager.h"
#import "IgnoredAreaView.h"
#import "KarabinerKit/KarabinerKit.h"
#import "MultitouchPrivate.h"
#import "NotificationKeys.h"
#import "PreferencesController.h"
#import "PreferencesKeys.h"
#import <pqrs/weakify.h>

static AppDelegate* global_self_ = nil;

@interface AppDelegate ()

@property(weak) IBOutlet IgnoredAreaView* ignoredAreaView;
@property(weak) IBOutlet PreferencesController* preferences;
@property(copy) NSArray* mtdevices;
@property FingerStatusManager* fingerStatusManager;
@property IONotificationPortRef notifyport;
@property CFRunLoopSourceRef loopsource;

@end

@implementation AppDelegate

- (instancetype)init {
  self = [super init];

  if (self) {
    _fingerStatusManager = [FingerStatusManager new];
  }

  return self;
}

// ------------------------------------------------------------
// Multitouch callback
static int callback(MTDeviceRef device, Finger* data, int fingers, double timestamp, int frame) {
  if (!data) {
    fingers = 0;
  }

  if (device) {
    [global_self_.fingerStatusManager update:device
                                        data:data
                                     fingers:fingers
                                   timestamp:timestamp
                                       frame:frame];
  }

  return 0;
}

- (void)setcallback:(BOOL)isset {
  @synchronized(self) {
    // ------------------------------------------------------------
    // unset callback (even if isset is YES.)
    if (self.mtdevices) {
      for (NSUInteger i = 0; i < [self.mtdevices count]; ++i) {
        MTDeviceRef device = (__bridge MTDeviceRef)(self.mtdevices[i]);
        if (!device) continue;

        MTDeviceStop(device, 0);
        MTUnregisterContactFrameCallback(device, callback);
      }
      self.mtdevices = nil;
    }

    // ------------------------------------------------------------
    // set callback if needed
    if (isset) {
      self.mtdevices = (NSArray*)CFBridgingRelease(MTDeviceCreateList());
      if (self.mtdevices) {
        for (NSUInteger i = 0; i < [self.mtdevices count]; ++i) {
          MTDeviceRef device = (__bridge MTDeviceRef)(self.mtdevices[i]);
          if (!device) continue;

          MTRegisterContactFrameCallback(device, callback);
          MTDeviceStart(device, 0);
        }
      }
    }
  }
}

// ------------------------------------------------------------
// IONotification
- (void)release_iterator:(io_iterator_t)iterator {
  for (;;) {
    io_object_t obj = IOIteratorNext(iterator);
    if (!obj) break;

    IOObjectRelease(obj);
  }
}

static void observer_IONotification(void* refcon, io_iterator_t iterator) {
  dispatch_async(dispatch_get_main_queue(), ^{
    // Relaunch when devices are plugged/unplugged.
    NSLog(@"observer_IONotification");
    [NSTask launchedTaskWithLaunchPath:[[NSBundle mainBundle] executablePath] arguments:@[]];
    [NSApp terminate:nil];
  });
}

- (void)unregisterIONotification {
  NSLog(@"unregisterIONotification");

  @synchronized(self) {
    if (self.notifyport) {
      if (self.loopsource) {
        CFRunLoopSourceInvalidate(self.loopsource);
        self.loopsource = nil;
      }
      IONotificationPortDestroy(self.notifyport);
      self.notifyport = nil;
    }
  }
}

- (void)registerIONotification {
  NSLog(@"registerIONotification");

  @synchronized(self) {
    if (self.notifyport) {
      [self unregisterIONotification];
    }

    self.notifyport = IONotificationPortCreate(kIOMasterPortDefault);
    if (!self.notifyport) {
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
      kr = IOServiceAddMatchingNotification(self.notifyport,
                                            kIOTerminatedNotification,
                                            (__bridge CFMutableDictionaryRef)(match),
                                            &observer_IONotification,
                                            (__bridge void*)(self),
                                            &it);
      if (kr != kIOReturnSuccess) {
        NSLog(@"[ERROR] IOServiceAddMatchingNotification");
        return;
      }
      [self release_iterator:it];
    }

    {
      // ------------------------------------------------------------
      NSMutableDictionary* match = (__bridge NSMutableDictionary*)(IOServiceMatching("AppleMultitouchDevice"));

      // ----------------------------------------------------------------------
      io_iterator_t it;
      kern_return_t kr;

      // for kIOMatchedNotification
      kr = IOServiceAddMatchingNotification(self.notifyport,
                                            kIOMatchedNotification,
                                            (__bridge CFMutableDictionaryRef)(match),
                                            &observer_IONotification,
                                            (__bridge void*)(self),
                                            &it);
      if (kr != kIOReturnSuccess) {
        NSLog(@"[ERROR] IOServiceAddMatchingNotification");
        return;
      }
      [self release_iterator:it];
    }

    // ----------------------------------------------------------------------
    self.loopsource = IONotificationPortGetRunLoopSource(self.notifyport);
    if (!self.loopsource) {
      NSLog(@"[ERROR] IONotificationPortGetRunLoopSource");
      return;
    }
    CFRunLoopAddSource(CFRunLoopGetCurrent(), self.loopsource, kCFRunLoopDefaultMode);
  }
}

// ------------------------------------------------------------
- (void)observer_NSWorkspaceDidWakeNotification:(NSNotification*)notification {
  @weakify(self);

  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(self);
    if (!self) return;

    NSLog(@"observer_NSWorkspaceDidWakeNotification");

    // sleep until devices are settled.
    [NSThread sleepForTimeInterval:1.0];

    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"relaunchAfterWakeUpFromSleep"]) {
      double wait = [[[NSUserDefaults standardUserDefaults] stringForKey:@"relaunchWait"] doubleValue];
      if (wait > 0) {
        [NSThread sleepForTimeInterval:wait];
      }

      [NSTask launchedTaskWithLaunchPath:[[NSBundle mainBundle] executablePath] arguments:@[]];
      [NSApp terminate:self];
    }

    [self setcallback:YES];
  });
}

- (void)registerWakeNotification {
  [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                         selector:@selector(observer_NSWorkspaceDidWakeNotification:)
                                                             name:NSWorkspaceDidWakeNotification
                                                           object:nil];
}

- (void)unregisterWakeNotification {
  [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self
                                                                name:NSWorkspaceDidWakeNotification
                                                              object:nil];
}

void enable(void) {
  [global_self_ registerIONotification];
  [global_self_ registerWakeNotification];

  // sleep until devices are settled.
  [NSThread sleepForTimeInterval:1.0];

  [global_self_ setcallback:YES];
}

void disable(void) {
  [global_self_ unregisterIONotification];
  [global_self_ unregisterWakeNotification];
  [global_self_ setcallback:NO];
}

- (void)setVariables {
  printf("touchedFingerCount: %d\n", (int)([self.fingerStatusManager getTouchedFixedFingerCount]));
}

// ------------------------------------------------------------
- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  [KarabinerKit setup];
  [KarabinerKit exitIfAnotherProcessIsRunning:"multitouch_extension.pid"];
  [KarabinerKit observeConsoleUserServerIsDisabledNotification];

  [[NSApplication sharedApplication] disableRelaunchOnLogin];

  // ----------------------------------------
  if (![[NSUserDefaults standardUserDefaults] boolForKey:@"hideIconInDock"]) {
    ProcessSerialNumber psn = {0, kCurrentProcess};
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
  }

  @weakify(self);
  [[NSNotificationCenter defaultCenter] addObserverForName:kFixedFingerStateChanged
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
                                                  @strongify(self);
                                                  if (!self) {
                                                    return;
                                                  }

                                                  [self setVariables];
                                                }];

  global_self_ = self;

  libkrbn_enable_grabber_client(enable,
                                disable,
                                disable);
}

- (void)dealloc {
  [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self];
  [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
}

// Note:
// We have to set NSSupportsSuddenTermination `NO` to use `applicationWillTerminate`.
- (void)applicationWillTerminate:(NSNotification*)aNotification {
  [self setcallback:NO];

  libkrbn_terminate();
}

- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApplication hasVisibleWindows:(BOOL)flag {
  [self.preferences show];
  return YES;
}

@end
