#import "PreferencesWindowController.h"
#import "LogFileTextViewController.h"

@interface PreferencesWindowController ()

@property (weak) IBOutlet LogFileTextViewController* grabberLogFileTextViewController;
@property (weak) IBOutlet LogFileTextViewController* consoleUserServerLogFileTextViewController;

@end

@implementation PreferencesWindowController

- (void)setup {
  [self.grabberLogFileTextViewController monitor:@"/var/log/karabiner_grabber_log.txt"];
  [self.consoleUserServerLogFileTextViewController monitor:[NSString stringWithFormat:@"%@/.karabiner.d/log/karabiner_console_user_server_log.txt", NSHomeDirectory()]];
}

- (void)show {
  [self.window makeKeyAndOrderFront:self];
  [NSApp activateIgnoringOtherApps:YES];
}

- (IBAction)launchUninstaller:(id)sender {
  NSString* path = @"/Library/Application Support/org.pqrs/Seil/uninstaller.applescript";
  [[[NSAppleScript alloc] initWithContentsOfURL:[NSURL fileURLWithPath:path] error:nil] executeAndReturnError:nil];
}

- (IBAction)openURL:(id)sender {
  [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[sender title]]];
}

@end
