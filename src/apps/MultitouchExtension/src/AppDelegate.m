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

@interface AppDelegate ()

@property(weak) IBOutlet PreferencesController* preferences;
@property KarabinerKitSmartObserverContainer* observers;

@end

@implementation AppDelegate

void enable(void) {
  MultitouchDeviceManager* manager = [MultitouchDeviceManager sharedMultitouchDeviceManager];

  [manager registerIONotification];
  [manager registerWakeNotification];

  // sleep until devices are settled.
  [NSThread sleepForTimeInterval:1.0];

  [manager setCallback:YES];
}

void disable(void) {
  MultitouchDeviceManager* manager = [MultitouchDeviceManager sharedMultitouchDeviceManager];

  [manager unregisterIONotification];
  [manager unregisterWakeNotification];

  [manager setCallback:NO];
}

- (void)setVariables {
  //
  // Prepare variable names
  //

  NSMutableSet<NSString*>* enableVariableNames = [NSMutableSet new];
  NSMutableSet<NSString*>* discardVariableNames = [NSMutableSet new];

  FingerStatusManager* manager = [FingerStatusManager sharedFingerStatusManager];
  NSUInteger fingerCount = [manager getTouchedFixedFingerCount];

  for (NSUInteger i = 1; i <= MAX_FINGER_COUNT; ++i) {
    if (![PreferencesController isSettingEnabled:i]) {
      continue;
    }

    NSString* name = [PreferencesController getSettingIdentifier:i];
    if (!name) {
      continue;
    }

    if (i == fingerCount) {
      [enableVariableNames addObject:name];
    } else {
      [discardVariableNames addObject:name];
    }
  }

  //
  // Unset variables
  //

  for (NSString* name in discardVariableNames) {
    if ([enableVariableNames containsObject:name]) {
      continue;
    }

    libkrbn_grabber_client_async_set_variable([name UTF8String], 0);
  }

  //
  // Set variables
  //

  for (NSString* name in enableVariableNames) {
    libkrbn_grabber_client_async_set_variable([name UTF8String], 1);
  }
}

// ------------------------------------------------------------
- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  [KarabinerKit setup];
  [KarabinerKit exitIfAnotherProcessIsRunning:"multitouch_extension.pid"];
  [KarabinerKit observeConsoleUserServerIsDisabledNotification];

  [[NSApplication sharedApplication] disableRelaunchOnLogin];

  //
  // hideIconInDock
  //

  if (![[NSUserDefaults standardUserDefaults] boolForKey:@"hideIconInDock"]) {
    ProcessSerialNumber psn = {0, kCurrentProcess};
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
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

                             [self setVariables];
                           }];
    [self.observers addObserver:o notificationCenter:center];
  }

  //
  // Enable grabber_client
  //

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
  [[MultitouchDeviceManager sharedMultitouchDeviceManager] setCallback:NO];

  libkrbn_terminate();
}

- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApplication hasVisibleWindows:(BOOL)flag {
  [self.preferences show];
  return YES;
}

@end
