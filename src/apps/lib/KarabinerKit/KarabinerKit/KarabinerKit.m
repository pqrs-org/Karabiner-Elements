#import "KarabinerKit.h"
#import "libkrbn.h"

static libkrbn_version_monitor* libkrbn_version_monitor_ = NULL;

static void version_changed_callback(void* refcon) {
  [KarabinerKit relaunch];
}

@implementation KarabinerKit

+ (void)setup {
  static dispatch_once_t once;

  dispatch_once(&once, ^{
    // initialize managers
    [KarabinerKitConfigurationManager sharedManager];
    [KarabinerKitDeviceManager sharedManager];

    libkrbn_version_monitor_initialize(&libkrbn_version_monitor_, version_changed_callback, NULL);
  });
}

+ (void)exitIfAnotherProcessIsRunning:(const char*)pidFileName {
  if (!libkrbn_lock_single_application_with_user_pid_file(pidFileName)) {
    NSLog(@"Exit since another process is running.");
    [NSApp terminate:nil];
  }
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
