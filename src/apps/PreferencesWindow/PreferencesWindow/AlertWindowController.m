#import "AlertWindowController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "libkrbn.h"

@interface AlertWindowController ()

@property(weak) IBOutlet NSWindow* preferencesWindow;
@property libkrbn_grabber_alerts_monitor* libkrbn_grabber_alerts_monitor;
@property BOOL shown;

- (void)callback;

@end

static void staticCallback(void* context) {
  AlertWindowController* p = (__bridge AlertWindowController*)(context);
  [p callback];
}

@implementation AlertWindowController

- (void)setup {
  libkrbn_grabber_alerts_monitor* p = nil;
  libkrbn_grabber_alerts_monitor_initialize(&p,
                                            staticCallback,
                                            (__bridge void*)(self));
  self.libkrbn_grabber_alerts_monitor = p;
}

- (void)dealloc {
  libkrbn_grabber_alerts_monitor* p = self.libkrbn_grabber_alerts_monitor;
  libkrbn_grabber_alerts_monitor_terminate(&p);
  self.libkrbn_grabber_alerts_monitor = nil;
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
  [self showIfNeeded];
}

- (IBAction)openSystemPreferencesSecurity:(id)sender {
  [[NSWorkspace sharedWorkspace] openFile:@"/System/Library/PreferencePanes/Security.prefPane"];
  [self.preferencesWindow endSheet:self.window];
  [NSApp terminate:nil];
}

- (IBAction)closePreferences:(id)sender {
  [self.preferencesWindow endSheet:self.window];
  [NSApp terminate:nil];
}

@end
