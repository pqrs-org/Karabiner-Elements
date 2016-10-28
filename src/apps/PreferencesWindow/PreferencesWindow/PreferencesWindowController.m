#import "PreferencesWindowController.h"
#import "DevicesTableViewController.h"
#import "FnFunctionKeysTableViewController.h"
#import "LogFileTextViewController.h"
#import "SimpleModificationsMenuManager.h"
#import "SimpleModificationsTableViewController.h"
#import "UpdaterController.h"

@interface PreferencesWindowController ()

@property(weak) IBOutlet DevicesTableViewController* devicesTableViewController;
@property(weak) IBOutlet FnFunctionKeysTableViewController* fnFunctionKeysTableViewController;
@property(weak) IBOutlet LogFileTextViewController* logFileTextViewController;
@property(weak) IBOutlet NSTableView* simpleModificationsTableView;
@property(weak) IBOutlet NSTableView* fnFunctionKeysTableView;
@property(weak) IBOutlet NSTableView* devicesTableView;
@property(weak) IBOutlet NSTextField* versionLabel;
@property(weak) IBOutlet SimpleModificationsMenuManager* simpleModificationsMenuManager;
@property(weak) IBOutlet SimpleModificationsTableViewController* simpleModificationsTableViewController;

@end

@implementation PreferencesWindowController

- (void)setup {
  [self.simpleModificationsMenuManager setup];
  [self.simpleModificationsTableViewController setup];
  [self.fnFunctionKeysTableViewController setup];
  [self.devicesTableViewController setup];

  self.versionLabel.stringValue = [[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"];

  [self.simpleModificationsTableView reloadData];
  [self.fnFunctionKeysTableView reloadData];
  [self.devicesTableView reloadData];
  [self.logFileTextViewController monitor];

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
    // If plistFilePath is already bootstrapped and disabled, launchctl bootstrap will fail until it is enabled again.
    // So we should enable it first, and then bootstrap and enable it.

    system([[NSString stringWithFormat:@"/bin/launchctl enable %@", serviceTarget] UTF8String]);
    system([[NSString stringWithFormat:@"/bin/launchctl bootstrap %@ %@", domainTarget, plistFilePath] UTF8String]);
    system([[NSString stringWithFormat:@"/bin/launchctl enable %@", serviceTarget] UTF8String]);

  } else {
    system([[NSString stringWithFormat:@"/bin/launchctl bootout %@ %@", domainTarget, plistFilePath] UTF8String]);
    system([[NSString stringWithFormat:@"/bin/launchctl disable %@", serviceTarget] UTF8String]);
  }
}

@end
