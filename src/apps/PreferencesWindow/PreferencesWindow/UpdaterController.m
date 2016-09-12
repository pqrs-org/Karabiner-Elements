#import "UpdaterController.h"

static dispatch_queue_t queue_;

@implementation UpdaterController

+ (void)initialize {
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    queue_ = dispatch_queue_create("org.pqrs.Karabiner-Elements.UpdaterController", NULL);
  });
}

+ (NSString*)path {
  return @"/Library/Application Support/org.pqrs/Karabiner-Elements/updater/Karabiner-Elements.app";
}

+ (void)terminateUpdaters {
  NSString* bundleIdentifier = [[NSBundle bundleWithPath:self.path] bundleIdentifier];
  NSArray* applications = [NSRunningApplication runningApplicationsWithBundleIdentifier:bundleIdentifier];
  for (NSRunningApplication* runningApplication in applications) {
    [runningApplication terminate];
  }
}

+ (void)launch:(NSString*)argument {
  [UpdaterController terminateUpdaters];
  [[NSWorkspace sharedWorkspace] launchApplicationAtURL:[NSURL fileURLWithPath:self.path]
                                                options:0
                                          configuration:@{ NSWorkspaceLaunchConfigurationArguments : @[ argument ] }
                                                  error:nil];
}

+ (void)checkForUpdatesInBackground {
  dispatch_sync(queue_, ^{
    [UpdaterController launch:@"checkForUpdatesInBackground"];
  });
}

+ (void)checkForUpdatesStableOnly {
  dispatch_sync(queue_, ^{
    [UpdaterController launch:@"checkForUpdatesStableOnly"];
  });
}

+ (void)checkForUpdatesWithBetaVersion {
  dispatch_sync(queue_, ^{
    [UpdaterController launch:@"checkForUpdatesWithBetaVersion"];
  });
}

@end
