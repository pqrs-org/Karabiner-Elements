#import "DextAlertWindowController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

@interface DextAlertWindowController ()

@property(weak) IBOutlet NSWindow* preferencesWindow;
@property BOOL shown;
@property NSTimer* timer;

@end

@implementation DextAlertWindowController

- (void)setup {
  @weakify(self);
  self.timer = [NSTimer timerWithTimeInterval:1.0
                                      repeats:YES
                                        block:^(NSTimer* timer) {
                                          @strongify(self);
                                          if (!self) {
                                            return;
                                          }

                                          if (libkrbn_driver_running()) {
                                            [self.timer invalidate];
                                            [self.preferencesWindow removeChildWindow:self.window];
                                            [self.window close];
                                            return;
                                          }

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
                                        }];
  [[NSRunLoop mainRunLoop] addTimer:self.timer forMode:NSRunLoopCommonModes];
  [self.timer fire];
}

- (IBAction)openSystemPreferencesSecurity:(id)sender {
  [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?General"]];
}

- (IBAction)deactivateDriver:(id)sender {
  system("/Applications/.Karabiner-VirtualHIDDevice-Manager.app/Contents/MacOS/Karabiner-VirtualHIDDevice-Manager deactivate &");
}

- (IBAction)activateDriver:(id)sender {
  system("/Applications/.Karabiner-VirtualHIDDevice-Manager.app/Contents/MacOS/Karabiner-VirtualHIDDevice-Manager activate &");
}

@end
