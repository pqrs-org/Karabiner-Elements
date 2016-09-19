#import "PreferencesWindowController.h"
#import "LogFileTextViewController.h"
#import "UpdaterController.h"

@interface PreferencesWindowController ()

@property(weak) IBOutlet LogFileTextViewController* consoleUserServerLogFileTextViewController;
@property(weak) IBOutlet LogFileTextViewController* grabberLogFileTextViewController;
@property(weak) IBOutlet NSTextField* versionLabel;

@end

@implementation PreferencesWindowController

- (void)setup {
  self.versionLabel.stringValue = [[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"];

  [self.grabberLogFileTextViewController monitor:@"/var/log/karabiner/grabber_log.txt"];
  [self.consoleUserServerLogFileTextViewController monitor:[NSString stringWithFormat:@"%@/.karabiner.d/log/console_user_server_log.txt", NSHomeDirectory()]];

  [self launchctlConsoleUserServer:YES];
}

- (void)show {
  [self.window makeKeyAndOrderFront:self];
  [NSApp activateIgnoringOtherApps:YES];
}

- (IBAction)checkForUpdatesStableOnly:(id)sender {
  [UpdaterController checkForUpdatesStableOnly];
}

- (IBAction)checkForUpdatesWithBetaVersion:(id)sender {
  [UpdaterController checkForUpdatesWithBetaVersion];
}

- (IBAction)launchUninstaller:(id)sender {
  NSString* path = @"/Library/Application Support/org.pqrs/Karabiner-Elements/uninstaller.applescript";
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
  uid_t uid = getuid();
  NSString* domainTarget = [NSString stringWithFormat:@"gui/%d", uid];
  NSString* serviceTarget = [NSString stringWithFormat:@"gui/%d/org.pqrs.karabiner.karabiner_console_user_server", uid];
  NSString* plistFilePath = @"/Library/LaunchAgents/org.pqrs.karabiner.karabiner_console_user_server.plist";

  if (load) {
    NSTask* task = nil;

    task = [NSTask launchedTaskWithLaunchPath:@"/bin/launchctl"
                                    arguments:@[ @"bootstrap", domainTarget, plistFilePath ]];
    [task waitUntilExit];

    task = [NSTask launchedTaskWithLaunchPath:@"/bin/launchctl"
                                    arguments:@[ @"enable", serviceTarget, plistFilePath ]];
    [task waitUntilExit];

    task = [NSTask launchedTaskWithLaunchPath:@"/bin/launchctl"
                                    arguments:@[ @"kickstart", serviceTarget, plistFilePath ]];
    [task waitUntilExit];

  } else {
    NSTask* task = [NSTask launchedTaskWithLaunchPath:@"/bin/launchctl"
                                            arguments:@[ @"bootout", domainTarget, plistFilePath ]];
    [task waitUntilExit];
  }
}

@end
