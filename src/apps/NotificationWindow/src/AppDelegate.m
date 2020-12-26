#import "AppDelegate.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationWindowManager.h"

@interface AppDelegate ()

@property NotificationWindowManager* notificationWindowManager;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
  [NSProcessInfo.processInfo enableSuddenTermination];

  [KarabinerKit setup];
  [KarabinerKit exitIfAnotherProcessIsRunning:"notification_window.pid"];
  [KarabinerKit observeConsoleUserServerIsDisabledNotification];

  self.notificationWindowManager = [NotificationWindowManager new];
}

- (void)applicationWillTerminate:(NSNotification*)notification {
  self.notificationWindowManager = nil;

  libkrbn_terminate();
}

@end
