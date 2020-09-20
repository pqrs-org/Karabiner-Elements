#import "InputMonitoringAlertWindowController.h"
#import "KarabinerKit/KarabinerKit.h"

@interface InputMonitoringAlertWindowController ()

@property(weak) IBOutlet NSWindow* parentWindow;

@end

@implementation InputMonitoringAlertWindowController

- (void)show {
  NSRect frame = NSMakeRect(
      self.parentWindow.frame.origin.x + (self.parentWindow.frame.size.width / 2) - (self.window.frame.size.width / 2),
      self.parentWindow.frame.origin.y + (self.parentWindow.frame.size.height / 2) - (self.window.frame.size.height / 2),
      self.window.frame.size.width,
      self.window.frame.size.height);

  [self.window setFrame:frame display:NO];
  [self.parentWindow addChildWindow:self.window
                            ordered:NSWindowAbove];
}

- (IBAction)openSystemPreferencesSecurity:(id)sender {
  [NSApplication.sharedApplication miniaturizeAll:self];
  [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy"]];
}

@end
