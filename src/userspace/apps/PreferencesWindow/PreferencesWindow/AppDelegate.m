#import "AppDelegate.h"
#import "PreferencesWindowController.h"

@interface AppDelegate ()

@property(weak) IBOutlet PreferencesWindowController* preferencesWindowController;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  [self.preferencesWindowController setup];
}

@end
