#import "InputMonitoringAlertWindowController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

@interface InputMonitoringAlertWindowController ()

@property(weak) IBOutlet NSWindow* preferencesWindow;
@property BOOL shown;
@property BOOL observerPermitted;
@property BOOL grabberPermitted;

- (void)observerStateJsonFileChangedCallback:(NSString*)filePath;
- (void)grabberStateJsonFileChangedCallback:(NSString*)filePath;

@end

static void staticObserverStateJsonFileChangedCallback(const char* filePath,
                                                       void* context) {
  InputMonitoringAlertWindowController* p = (__bridge InputMonitoringAlertWindowController*)(context);
  if (p) {
    [p observerStateJsonFileChangedCallback:[NSString stringWithUTF8String:filePath]];
  }
}

static void staticGrabberStateJsonFileChangedCallback(const char* filePath,
                                                      void* context) {
  InputMonitoringAlertWindowController* p = (__bridge InputMonitoringAlertWindowController*)(context);
  if (p) {
    [p grabberStateJsonFileChangedCallback:[NSString stringWithUTF8String:filePath]];
  }
}

@implementation InputMonitoringAlertWindowController

- (void)setup {
  self.observerPermitted = YES;
  self.grabberPermitted = YES;

  libkrbn_enable_observer_state_json_file_monitor(staticObserverStateJsonFileChangedCallback,
                                                  (__bridge void*)(self));

  libkrbn_enable_grabber_state_json_file_monitor(staticGrabberStateJsonFileChangedCallback,
                                                 (__bridge void*)(self));
}

- (void)dealloc {
  libkrbn_disable_observer_state_json_file_monitor();

  libkrbn_disable_grabber_state_json_file_monitor();
}

- (void)observerStateJsonFileChangedCallback:(NSString*)filePath {
  NSDictionary* jsonObject = [KarabinerKitJsonUtility loadFile:filePath];
  if (jsonObject) {
    NSString* result = jsonObject[@"hid_device_open_permitted"];
    if ([result isKindOfClass:[NSNumber class]]) {
      if (!result.boolValue) {
        self.observerPermitted = NO;
        [self manageAlert];
        return;
      }
    }
  }

  self.observerPermitted = YES;
  [self manageAlert];
}

- (void)grabberStateJsonFileChangedCallback:(NSString*)filePath {
  NSDictionary* jsonObject = [KarabinerKitJsonUtility loadFile:filePath];
  if (jsonObject) {
    NSString* result = jsonObject[@"hid_device_open_permitted"];
    if ([result isKindOfClass:[NSNumber class]]) {
      if (!result.boolValue) {
        self.grabberPermitted = NO;
        [self manageAlert];
        return;
      }
    }
  }

  self.grabberPermitted = YES;
  [self manageAlert];
}

- (void)manageAlert {
  @weakify(self);
  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(self);
    if (!self) {
      return;
    }

    if (!self.observerPermitted ||
        !self.grabberPermitted) {
      if (!self.shown) {
        self.shown = YES;

        NSRect frame = NSMakeRect(
            self.preferencesWindow.frame.origin.x + (self.preferencesWindow.frame.size.width / 2) - (self.window.frame.size.width / 2),
            self.preferencesWindow.frame.origin.y + (self.preferencesWindow.frame.size.height / 2) - (self.window.frame.size.height / 2),
            self.window.frame.size.width,
            self.window.frame.size.height);

        [self.window setFrame:frame display:NO];
        [self.preferencesWindow addChildWindow:self.window
                                       ordered:NSWindowAbove];
      }
      return;
    }

    if (self.shown) {
      [self.preferencesWindow removeChildWindow:self.window];
      [self.window close];
    }
  });
}

- (IBAction)openSystemPreferencesSecurity:(id)sender {
  [NSApplication.sharedApplication miniaturizeAll:self];
  [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy"]];
}

@end
