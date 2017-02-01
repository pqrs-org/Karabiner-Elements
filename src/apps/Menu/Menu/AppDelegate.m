#import "AppDelegate.h"
#import "KarabinerKit/KarabinerKit.h"
#import "MenuController.h"

@interface AppDelegate ()

@property(weak) IBOutlet MenuController* menuController;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  [KarabinerKit setup];
  [KarabinerKit exitIfAnotherProcessIsRunning:"menu.pid"];

  [self.menuController setup];
}

@end
