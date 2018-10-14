#import "AppDelegate.h"
#import "KarabinerKit/KarabinerKit.h"
#import "MenuController.h"

@interface AppDelegate ()

@property(weak) IBOutlet MenuController* menuController;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
  [KarabinerKit setup];
  [KarabinerKit exitIfAnotherProcessIsRunning:"menu.pid"];
  [KarabinerKit observeConsoleUserServerIsDisabledNotification];

  [self.menuController setup];
}

- (void)applicationWillTerminate:(NSNotification*)notification {
  libkrbn_terminate();
}

@end
