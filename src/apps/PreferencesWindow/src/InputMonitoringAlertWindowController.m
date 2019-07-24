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
    NSString* result = jsonObject[@"error"];
    if ([result isKindOfClass:[NSString class]]) {
      if ([result isEqualToString:@"hid_device_open_not_permitted"]) {
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
    NSString* result = jsonObject[@"error"];
    if ([result isKindOfClass:[NSString class]]) {
      if ([result isEqualToString:@"hid_device_open_not_permitted"]) {
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
        [self.preferencesWindow beginSheet:self.window
                         completionHandler:^(NSModalResponse returnCode){}];
        return;
      }
    }

    [self.preferencesWindow endSheet:self.window];
    self.shown = NO;
  });
}

- (IBAction)openSystemPreferencesSecurity:(id)sender {
  [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy"]];
}

- (IBAction)closeAlert:(id)sender {
  [self.preferencesWindow endSheet:self.window];
}

@end
