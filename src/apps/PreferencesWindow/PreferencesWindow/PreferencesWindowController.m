#import "PreferencesWindowController.h"
#import "DevicesTableViewController.h"
#import "FnFunctionKeysTableViewController.h"
#import "LogFileTextViewController.h"
#import "NotificationKeys.h"
#import "SimpleModificationsMenuManager.h"
#import "SimpleModificationsTableViewController.h"
#import "SystemPreferencesManager.h"
#import "UpdaterController.h"
#import "weakify.h"

@interface PreferencesWindowController ()

@property(weak) IBOutlet DevicesTableViewController* devicesTableViewController;
@property(weak) IBOutlet FnFunctionKeysTableViewController* fnFunctionKeysTableViewController;
@property(weak) IBOutlet LogFileTextViewController* logFileTextViewController;
@property(weak) IBOutlet NSButton* keyboardFnStateButton;
@property(weak) IBOutlet NSStepper* initialKeyRepeatStepper;
@property(weak) IBOutlet NSStepper* keyRepeatStepper;
@property(weak) IBOutlet NSTableView* devicesTableView;
@property(weak) IBOutlet NSTableView* devicesExternalKeyboardTableView;
@property(weak) IBOutlet NSTableView* fnFunctionKeysTableView;
@property(weak) IBOutlet NSTableView* simpleModificationsTableView;
@property(weak) IBOutlet NSTextField* initialKeyRepeatTextField;
@property(weak) IBOutlet NSTextField* keyRepeatTextField;
@property(weak) IBOutlet NSTextField* versionLabel;
@property(weak) IBOutlet SimpleModificationsMenuManager* simpleModificationsMenuManager;
@property(weak) IBOutlet SimpleModificationsTableViewController* simpleModificationsTableViewController;
@property(weak) IBOutlet SystemPreferencesManager* systemPreferencesManager;

@end

@implementation PreferencesWindowController

- (void)setup {
  // ----------------------------------------
  // Setup

  [self.simpleModificationsMenuManager setup];
  [self.simpleModificationsTableViewController setup];
  [self.fnFunctionKeysTableViewController setup];
  [self.devicesTableViewController setup];
  [self.logFileTextViewController monitor];

  @weakify(self);
  [[NSNotificationCenter defaultCenter] addObserverForName:kSystemPreferencesValuesAreUpdated
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
                                                  @strongify(self);
                                                  if (!self) return;

                                                  [self updateSystemPreferencesUIValues];
                                                }];

  // ----------------------------------------
  // Update UI values

  self.versionLabel.stringValue = [[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"];

  [self.simpleModificationsTableView reloadData];
  [self.fnFunctionKeysTableView reloadData];
  [self.devicesTableView reloadData];
  [self.devicesExternalKeyboardTableView reloadData];

  [self updateSystemPreferencesUIValues];

  // ----------------------------------------
  [self launchctlConsoleUserServer:YES];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)show {
  [self.window makeKeyAndOrderFront:self];
  [NSApp activateIgnoringOtherApps:YES];
}

- (void)updateSystemPreferencesUIValues {
  self.keyboardFnStateButton.state = self.systemPreferencesManager.systemPreferencesModel.keyboardFnState ? NSOnState : NSOffState;

  uint32_t initialKeyRepeatMilliseconds = self.systemPreferencesManager.systemPreferencesModel.initialKeyRepeatMilliseconds;
  self.initialKeyRepeatTextField.stringValue = [NSString stringWithFormat:@"%d", initialKeyRepeatMilliseconds];
  self.initialKeyRepeatStepper.integerValue = initialKeyRepeatMilliseconds;

  uint32_t keyRepeatMilliseconds = self.systemPreferencesManager.systemPreferencesModel.keyRepeatMilliseconds;
  self.keyRepeatTextField.stringValue = [NSString stringWithFormat:@"%d", keyRepeatMilliseconds];
  self.keyRepeatStepper.integerValue = keyRepeatMilliseconds;
}

- (IBAction)updateSystemPreferencesValues:(id)sender {
  SystemPreferencesModel* model = self.systemPreferencesManager.systemPreferencesModel;

  if (sender == self.keyboardFnStateButton) {
    model.keyboardFnState = (self.keyboardFnStateButton.state == NSOnState);
  }
  if (sender == self.initialKeyRepeatTextField) {
    model.initialKeyRepeatMilliseconds = [self.initialKeyRepeatTextField.stringValue intValue];
  }
  if (sender == self.initialKeyRepeatStepper) {
    model.initialKeyRepeatMilliseconds = self.initialKeyRepeatStepper.intValue;
  }
  if (sender == self.keyRepeatTextField) {
    model.keyRepeatMilliseconds = [self.keyRepeatTextField.stringValue intValue];
  }
  if (sender == self.keyRepeatStepper) {
    model.keyRepeatMilliseconds = self.keyRepeatStepper.intValue;
  }

  [self updateSystemPreferencesUIValues];
  [self.systemPreferencesManager updateSystemPreferencesValues:model];
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
