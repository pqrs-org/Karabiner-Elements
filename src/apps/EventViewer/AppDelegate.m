@import Carbon;
#import "AppDelegate.h"
#import "KeyResponder.h"
#import "PreferencesKeys.h"
#import "weakify.h"

@interface AppDelegate ()

@property(weak) IBOutlet NSWindow* window;
@property(weak) IBOutlet KeyResponder* keyResponder;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  [self setKeyResponder];
  [self setWindowProperty:self];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)theApplication {
  return YES;
}

- (void)dealloc {
  [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
}

- (void)tabView:(NSTabView*)tabView didSelectTabViewItem:(NSTabViewItem*)tabViewItem {
  if ([[tabViewItem identifier] isEqualToString:@"Main"]) {
    [self setKeyResponder];
  }
}

- (void)setKeyResponder {
  [self.window makeFirstResponder:self.keyResponder];
}

- (IBAction)setWindowProperty:(id)sender {
  // ----------------------------------------
  if ([[NSUserDefaults standardUserDefaults] boolForKey:kForceStayTop]) {
    [self.window setLevel:NSFloatingWindowLevel];

    NSWindowCollectionBehavior behavior = [self.window collectionBehavior];
    behavior &= ~(NSWindowCollectionBehaviorTransient);
    behavior |= NSWindowCollectionBehaviorManaged;
    [self.window setCollectionBehavior:behavior];

  } else {
    [self.window setLevel:NSNormalWindowLevel];

    NSWindowCollectionBehavior behavior = [self.window collectionBehavior];
    behavior &= ~(NSWindowCollectionBehaviorTransient);
    behavior |= NSWindowCollectionBehaviorManaged;
    [self.window setCollectionBehavior:behavior];
  }

  // ----------------------------------------
  if ([[NSUserDefaults standardUserDefaults] boolForKey:kShowInAllSpaces]) {
    NSWindowCollectionBehavior behavior = [self.window collectionBehavior];
    behavior &= ~(NSWindowCollectionBehaviorMoveToActiveSpace);
    behavior |= NSWindowCollectionBehaviorCanJoinAllSpaces;
    [self.window setCollectionBehavior:behavior];

  } else {
    NSWindowCollectionBehavior behavior = [self.window collectionBehavior];
    behavior &= ~(NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorMoveToActiveSpace);
    [self.window setCollectionBehavior:behavior];
  }
}

@end
