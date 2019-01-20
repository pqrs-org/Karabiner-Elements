#import "AlertWindowController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

@interface AlertWindowController ()

@property(weak) IBOutlet NSWindow* preferencesWindow;
@property BOOL shown;

- (void)callback:(NSString*)filePath;

@end

static void staticCallback(const char* filePath,
                           void* context) {
  AlertWindowController* p = (__bridge AlertWindowController*)(context);
  if (p && filePath) {
    [p callback:[NSString stringWithUTF8String:filePath]];
  }
}

@implementation AlertWindowController

- (void)setup {
  libkrbn_enable_grabber_alerts_json_file_monitor(staticCallback,
                                                  (__bridge void*)(self));
}

- (void)dealloc {
  libkrbn_disable_grabber_alerts_json_file_monitor();
}

- (void)showIfNeeded:(NSString*)filePath {
  NSDictionary* json = [KarabinerKitJsonUtility loadFile:filePath];
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

- (void)callback:(NSString*)filePath {
  @weakify(self);
  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(self);
    if (!self) {
      return;
    }

    [self showIfNeeded:filePath];
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
