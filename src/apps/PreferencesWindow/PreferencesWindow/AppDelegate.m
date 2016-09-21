#import "AppDelegate.h"
#import "PreferencesWindowController.h"
#import "Relauncher.h"
#include "libkrbn.h"

@interface AppDelegate ()

@property(weak) IBOutlet PreferencesWindowController* preferencesWindowController;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  [[NSApplication sharedApplication] disableRelaunchOnLogin];

  [self.preferencesWindowController setup];

  [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                      selector:@selector(grabberIsLaunchedNotificationCallback:)
                                                          name:[NSString stringWithUTF8String:libkrbn_get_distributed_notification_grabber_is_launched()]
                                                        object:nil
                                            suspensionBehavior:NSNotificationSuspensionBehaviorDeliverImmediately];

  system("open '/Library/Application Support/org.pqrs/Karabiner-Elements/updater/Karabiner-Elements.app'");
}

- (void)dealloc {
  [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)theApplication {
  return YES;
}

- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApplication hasVisibleWindows:(BOOL)flag {
  [self.preferencesWindowController show];
  return YES;
}

- (void)grabberIsLaunchedNotificationCallback:(NSNotification*)notification {
  dispatch_async(dispatch_get_main_queue(), ^{
    [Relauncher relaunch];
  });
}

@end
