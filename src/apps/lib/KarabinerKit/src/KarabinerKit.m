#import "KarabinerKit/KarabinerKit.h"
#import "libkrbn.h"

static libkrbn_version_monitor* libkrbn_version_monitor_ = NULL;

static void version_changed_callback(void* refcon) {
  dispatch_async(dispatch_get_main_queue(), ^{
    [KarabinerKit relaunch];
  });
}

@implementation KarabinerKit

+ (void)setup {
  static dispatch_once_t once;

  dispatch_once(&once, ^{
    // initialize managers
    [KarabinerKitConfigurationManager sharedManager];
    [KarabinerKitDeviceManager sharedManager];

    libkrbn_version_monitor_initialize(&libkrbn_version_monitor_, version_changed_callback, NULL);
    [self observeGrabberIsLaunchedNotification];
  });
}

+ (void)exitIfAnotherProcessIsRunning:(const char*)pidFileName {
  if (!libkrbn_lock_single_application_with_user_pid_file(pidFileName)) {
    NSLog(@"Exit since another process is running.");
    [NSApp terminate:nil];
  }
}

+ (void)endAllAttachedSheets:(NSWindow*)window {
  for (;;) {
    NSWindow* sheet = window.attachedSheet;
    if (!sheet) {
      break;
    }

    [self endAllAttachedSheets:sheet];
    [window endSheet:sheet];
  }
}

+ (void)observeConsoleUserServerIsDisabledNotification {
  NSString* name = [NSString stringWithUTF8String:libkrbn_get_distributed_notification_console_user_server_is_disabled()];
  NSString* object = [NSString stringWithUTF8String:libkrbn_get_distributed_notification_observed_object()];

  [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                      selector:@selector(consoleUserServerIsDisabledCallback)
                                                          name:name
                                                        object:object
                                            suspensionBehavior:NSNotificationSuspensionBehaviorDeliverImmediately];
}

+ (void)consoleUserServerIsDisabledCallback {
  [NSApp terminate:nil];
}

+ (void)observeGrabberIsLaunchedNotification {
  NSString* name = [NSString stringWithUTF8String:libkrbn_get_distributed_notification_grabber_is_launched()];
  NSString* object = [NSString stringWithUTF8String:libkrbn_get_distributed_notification_observed_object()];

  [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                      selector:@selector(grabberIsLaunchedCallback)
                                                          name:name
                                                        object:object
                                            suspensionBehavior:NSNotificationSuspensionBehaviorDeliverImmediately];
}

+ (void)grabberIsLaunchedCallback {
  [KarabinerKit relaunch];
}

+ (void)relaunch {
  libkrbn_unlock_single_application();

  [NSTask launchedTaskWithLaunchPath:[[NSBundle mainBundle] executablePath] arguments:@[]];
  [NSApp terminate:nil];
}

+ (BOOL)quitKarabinerWithConfirmation {
  NSAlert* alert = [NSAlert new];
  alert.messageText = @"Are you sure you want to quit Karabiner-Elements?";
  alert.informativeText = @"The changed key will be restored after Karabiner-Elements is quit.";
  [alert addButtonWithTitle:@"Quit"];
  [alert addButtonWithTitle:@"Cancel"];
  if ([alert runModal] == NSAlertFirstButtonReturn) {
    libkrbn_launchctl_manage_console_user_server(false);
    return YES;
  }
  return NO;
}

@end
