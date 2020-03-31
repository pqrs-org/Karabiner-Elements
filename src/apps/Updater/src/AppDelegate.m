#import "AppDelegate.h"
#import <pqrs/weakify.h>
@import Sparkle;

@interface AppDelegate ()

@property SUUpdater* suUpdater;
@property dispatch_source_t timer;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  self.suUpdater = [SUUpdater new];

  NSString* mode = @"checkForUpdatesInBackground";

  NSArray* arguments = [[NSProcessInfo processInfo] arguments];
  if ([arguments count] == 2) {
    mode = arguments[1];
  }

  if ([mode isEqualToString:@"checkForUpdatesStableOnly"]) {
    [self checkForUpdatesStableOnly];
  } else if ([mode isEqualToString:@"checkForUpdatesWithBetaVersion"]) {
    [self checkForUpdatesWithBetaVersion];
  } else {
    [self checkForUpdatesInBackground];
  }
}

- (NSString*)getFeedURL:(BOOL)includingBetaVersions {
  // ----------------------------------------
  // check beta & stable releases.

  // Once we check appcast.xml, SUFeedURL is stored in a user's preference file.
  // So that Sparkle gives priority to a preference over Info.plist,
  // we overwrite SUFeedURL here.
  if (includingBetaVersions) {
    return @"https://appcast.pqrs.org/karabiner-elements-appcast-devel.xml";
  }

  return @"https://appcast.pqrs.org/karabiner-elements-appcast.xml";
}

- (void)checkForUpdatesInBackground {
  NSString* url = [self getFeedURL:NO];
  [self.suUpdater setFeedURL:[NSURL URLWithString:url]];
  NSLog(@"checkForUpdates %@", url);
  [self.suUpdater checkForUpdatesInBackground];
  [self setTerminationTimer];
}

- (void)checkForUpdatesStableOnly {
  NSString* url = [self getFeedURL:NO];
  [self.suUpdater setFeedURL:[NSURL URLWithString:url]];
  NSLog(@"checkForUpdates %@", url);
  [self.suUpdater checkForUpdates:nil];
  [self setTerminationTimer];
}

- (void)checkForUpdatesWithBetaVersion {
  NSString* url = [self getFeedURL:YES];
  [self.suUpdater setFeedURL:[NSURL URLWithString:url]];
  NSLog(@"checkForUpdates %@", url);
  [self.suUpdater checkForUpdates:nil];
  [self setTerminationTimer];
}

- (void)setTerminationTimer {
  @weakify(self);

  self.timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
  if (self.timer) {
    dispatch_source_set_timer(self.timer, dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC), 1.0 * NSEC_PER_SEC, 0);
    dispatch_source_set_event_handler(self.timer, ^{
      @strongify(self);
      if (!self) return;

      if (!self.suUpdater.updateInProgress) {
        [NSApp terminate:self];
      }
    });
    dispatch_resume(self.timer);
  }
}

@end
