@import Carbon;
#import "AppDelegate.h"
#import "EventQueue.h"
#import "KarabinerKit/KarabinerKit.h"
#import "Karabiner_EventViewer-Swift.h"
#import "KeyResponder.h"
#import "PreferencesKeys.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

@interface AppDelegate ()

@property(weak) IBOutlet NSWindow* window;
@property(weak) IBOutlet EventQueue* eventQueue;
@property(weak) IBOutlet KeyResponder* keyResponder;
@property(weak) IBOutlet FrontmostApplicationController* frontmostApplicationController;
@property(weak) IBOutlet VariablesController* variablesController;
@property(weak) IBOutlet DevicesController* devicesController;
@property(weak) IBOutlet InputMonitoringAlertWindowController* inputMonitoringAlertWindowController;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  [KarabinerKit setup];

  [self setKeyResponder];
  [self setWindowProperty:self];
  [self.eventQueue setup];
  [self.frontmostApplicationController setup];
  [self.variablesController setup];
  [self.devicesController setup];

  @weakify(self);
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 3 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    @strongify(self);
    if (!self) {
      return;
    }

    BOOL observed = self.eventQueue.observed;
    NSLog(@"observed = %d", observed);

    if (!observed) {
      [self.inputMonitoringAlertWindowController show];
    }
  });
}

// Note:
// We have to set NSSupportsSuddenTermination `NO` to use `applicationWillTerminate`.
- (void)applicationWillTerminate:(NSNotification*)notification {
  libkrbn_terminate();
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)theApplication {
  return YES;
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
