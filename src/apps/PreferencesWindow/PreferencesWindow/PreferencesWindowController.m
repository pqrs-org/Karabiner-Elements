#import "PreferencesWindowController.h"
#import "LogFileTextViewController.h"

@interface PreferencesWindowController ()

@property(weak) IBOutlet LogFileTextViewController* grabberLogFileTextViewController;
@property(weak) IBOutlet LogFileTextViewController* consoleUserServerLogFileTextViewController;

@end

@implementation PreferencesWindowController

- (void)setup {
  [self.grabberLogFileTextViewController monitor:@"/var/log/karabiner/grabber_log.txt"];
  [self.consoleUserServerLogFileTextViewController monitor:[NSString stringWithFormat:@"%@/.karabiner.d/log/console_user_server_log.txt", NSHomeDirectory()]];

  [self launchctlConsoleUserServer:YES];
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

- (IBAction)quitWithConfirmation:(id)sender {
  NSAlert* alert = [NSAlert new];
  alert.messageText = @"Are you sure you want to quit Karabiner-Elements?";
  alert.informativeText = @"The changed key will be restored after Karabiner-Elements is quit.";
  [alert addButtonWithTitle:@"Quit"];
  [alert addButtonWithTitle:@"Cancel"];
  if ([alert runModal] == NSAlertFirstButtonReturn) {
    [self launchctlConsoleUserServer:NO];
    [NSApp terminate:nil];
  }
}

- (void)launchctlConsoleUserServer:(BOOL)load {
  NSTask* task = [NSTask launchedTaskWithLaunchPath:@"/bin/launchctl"
                                          arguments:@[
                                            load ? @"load" : @"unload",
                                            @"-w",
                                            @"/Library/LaunchAgents/org.pqrs.karabiner.karabiner_console_user_server.plist",
                                          ]];
  [task waitUntilExit];
}

@end
