#import "AppDelegate.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationWindowManager.h"

@interface AppDelegate ()

@property NotificationWindowManager* notificationWindowManager;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
  [KarabinerKit setup];
  [KarabinerKit exitIfAnotherProcessIsRunning:"notification_window.pid"];
  [KarabinerKit observeConsoleUserServerIsDisabledNotification];
}

// Note:
// We have to set NSSupportsSuddenTermination `NO` to use `applicationWillTerminate`.
- (void)applicationWillTerminate:(NSNotification*)notification {
  libkrbn_terminate();
}

@end
