@import IOKit;
#import "AppDelegate.h"
#import "FingerStatusManager.h"
#import "KarabinerKit/KarabinerKit.h"
#import "MultitouchDeviceManager.h"
#import "MultitouchPrivate.h"
#import "NotificationKeys.h"
#import "PreferencesController.h"
#import "PreferencesKeys.h"
#import <pqrs/weakify.h>

//
// C methods
//

static void setGrabberVariable(int fingerCount, bool sync) {
  const char* name = "multitouch_extension_finger_count";
  if (sync) {
    libkrbn_grabber_client_sync_set_variable(name, fingerCount);
  } else {
    libkrbn_grabber_client_async_set_variable(name, fingerCount);
  }
}

static void enable(void) {
  MultitouchDeviceManager* manager = [MultitouchDeviceManager sharedMultitouchDeviceManager];

  [manager registerIONotification];
  [manager registerWakeNotification];

  // sleep until devices are settled.
  [NSThread sleepForTimeInterval:1.0];

  [manager setCallback:YES];

  setGrabberVariable(0, false);
}

static void disable(void) {
  MultitouchDeviceManager* manager = [MultitouchDeviceManager sharedMultitouchDeviceManager];

  [manager unregisterIONotification];
  [manager unregisterWakeNotification];

  [manager setCallback:NO];
}

//
// AppDelegate
//

@interface AppDelegate ()

@property(weak) IBOutlet PreferencesController* preferences;
@property KarabinerKitSmartObserverContainer* observers;
@property id activity;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  [KarabinerKit setup];
  [KarabinerKit observeConsoleUserServerIsDisabledNotification];

  [[NSApplication sharedApplication] disableRelaunchOnLogin];

  //
  // Handle kHideIconInDock
  //

  if (![[NSUserDefaults standardUserDefaults] boolForKey:kHideIconInDock]) {
    ProcessSerialNumber psn = {0, kCurrentProcess};
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
  }

  //
  // Handle --start-at-login
  //

  if (self.startAtLogin) {
    if (![[NSUserDefaults standardUserDefaults] boolForKey:kStartAtLogin]) {
      [NSApp terminate:nil];
    }
  }

  //
  // Handle --show-ui
  //

  if (self.showUI) {
    [self.preferences show];
  }

  //
  // Prepare observers
  //

  self.observers = [KarabinerKitSmartObserverContainer new];
  @weakify(self);

  {
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    id o = [center addObserverForName:kFixedFingerStateChanged
                               object:nil
                                queue:[NSOperationQueue mainQueue]
                           usingBlock:^(NSNotification* note) {
                             @strongify(self);
                             if (!self) {
                               return;
                             }

                             FingerStatusManager* manager = [FingerStatusManager sharedFingerStatusManager];
                             NSUInteger fingerCount = [manager getTouchedFixedFingerCount];
                             setGrabberVariable(fingerCount, false);
                           }];
    [self.observers addObserver:o notificationCenter:center];
  }

  //
  // Enable grabber_client
  //

  libkrbn_enable_grabber_client(enable,
                                disable,
                                disable);

  //
  // Disable App Nap
  //

  NSActivityOptions options = NSActivityUserInitiatedAllowingIdleSystemSleep;
  NSString* reason = @"Disable App Nap in order to receive multitouch events even if this app is background";
  self.activity = [[NSProcessInfo processInfo] beginActivityWithOptions:options reason:reason];
}

// Note:
// We have to set NSSupportsSuddenTermination `NO` to use `applicationWillTerminate`.
- (void)applicationWillTerminate:(NSNotification*)aNotification {
  if (self.activity) {
    [[NSProcessInfo processInfo] endActivity:self.activity];
    self.activity = nil;
  }

  [[MultitouchDeviceManager sharedMultitouchDeviceManager] setCallback:NO];

  setGrabberVariable(0, true);

  libkrbn_terminate();
}

- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApplication hasVisibleWindows:(BOOL)flag {
  [self.preferences show];
  return YES;
}

- (BOOL)startAtLogin {
  return [[[NSProcessInfo processInfo] arguments] indexOfObject:@"--start-at-login"] != NSNotFound;
}

- (BOOL)showUI {
  return [[[NSProcessInfo processInfo] arguments] indexOfObject:@"--show-ui"] != NSNotFound;
}

@end
