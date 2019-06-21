#import "AlertWindowController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "libkrbn/libkrbn.h"
#import <libkern/OSKextLib.h>
#import <pqrs/weakify.h>

@interface AlertWindowController ()

@property(weak) IBOutlet NSWindow* preferencesWindow;
@property BOOL shown;

- (void)kextLoadResultChangedCallback:(kern_return_t)kr;

@end

static void staticKextLoadResultChangedCallback(kern_return_t kr,
                                                void* context) {
  AlertWindowController* p = (__bridge AlertWindowController*)(context);
  if (p) {
    [p kextLoadResultChangedCallback:kr];
  }
}

@implementation AlertWindowController

- (void)setup {
  libkrbn_enable_kextd_state_monitor(staticKextLoadResultChangedCallback,
                                     (__bridge void*)(self));
}

- (void)dealloc {
  libkrbn_disable_kextd_state_monitor();
}

- (void)showIfNeeded:(kern_return_t)kr {
  if (kr == kOSKextReturnSystemPolicy) {
    if (!self.shown) {
      self.shown = YES;
      [self.preferencesWindow beginSheet:self.window
                       completionHandler:^(NSModalResponse returnCode){}];
      return;
    }
  }

  [self.preferencesWindow endSheet:self.window];
  self.shown = NO;
}

- (void)kextLoadResultChangedCallback:(kern_return_t)kr {
  @weakify(self);
  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(self);
    if (!self) {
      return;
    }

    [self showIfNeeded:kr];
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
