#import "AppDelegate.h"
#import "KarabinerKit/KarabinerKit.h"
#import "PreferencesWindowController.h"
#import "Relauncher.h"
#import "SystemPreferencesManager.h"
#import "libkrbn.h"

@interface AppDelegate ()

@property(weak) IBOutlet PreferencesWindowController* preferencesWindowController;
@property(weak) IBOutlet SystemPreferencesManager* systemPreferencesManager;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  [[NSApplication sharedApplication] disableRelaunchOnLogin];

  [KarabinerKit setup];

  [self.systemPreferencesManager setup];

  [self.preferencesWindowController setup];

  [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                      selector:@selector(grabberIsLaunchedNotificationCallback:)
                                                          name:[NSString stringWithUTF8String:libkrbn_get_distributed_notification_grabber_is_launched()]
                                                        object:[NSString stringWithUTF8String:libkrbn_get_distributed_notification_observed_object()]
                                            suspensionBehavior:NSNotificationSuspensionBehaviorDeliverImmediately];
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
