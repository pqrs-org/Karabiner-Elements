#import "AlertWindowController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

@interface AlertWindowController ()

@property(weak) IBOutlet NSWindow* preferencesWindow;
@property libkrbn_file_monitor* libkrbn_file_monitor;
@property BOOL shown;

- (void)callback;

@end

static void staticCallback(void* context) {
  AlertWindowController* p = (__bridge AlertWindowController*)(context);
  [p callback];
}

@implementation AlertWindowController

- (void)setup {
  libkrbn_file_monitor* p = nil;
  libkrbn_file_monitor_initialize(&p,
                                  libkrbn_get_grabber_alerts_json_file_path(),
                                  staticCallback,
                                  (__bridge void*)(self));
  self.libkrbn_file_monitor = p;
}

- (void)dealloc {
  libkrbn_file_monitor* p = self.libkrbn_file_monitor;
  libkrbn_file_monitor_terminate(&p);
  self.libkrbn_file_monitor = nil;
}

- (void)showIfNeeded {
  NSString* alertsJsonFilePath = [NSString stringWithUTF8String:libkrbn_get_grabber_alerts_json_file_path()];

  NSDictionary* json = [KarabinerKitJsonUtility loadFile:alertsJsonFilePath];
  for (NSString* alert in json[@"alerts"]) {
    if ([alert isEqualToString:@"system_policy_prevents_loading_kext"]) {
      if (!self.shown) {
        self.shown = YES;
        [self.preferencesWindow beginSheet:self.window
                         completionHandler:^(NSModalResponse returnCode){}];
      }
      return;
    }
  }

  [self.preferencesWindow endSheet:self.window];
  self.shown = NO;
}

- (void)callback {
  @weakify(self);
  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(self);
    if (!self) {
      return;
    }

    [self showIfNeeded];
  });
}

- (IBAction)openSystemPreferencesSecurity:(id)sender {
  [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?General"]];
}

- (IBAction)closePreferences:(id)sender {
  [self.preferencesWindow endSheet:self.window];
  [NSApp terminate:nil];
}

- (IBAction)openSystemPolicyPreventsLoadingKextHelpWebPage:(id)sender {
  [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://pqrs.org/osx/karabiner/help.html#kext-allow-button-does-not-work"]];
}

@end
