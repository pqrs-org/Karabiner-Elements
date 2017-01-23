#import "PreferencesWindowController.h"
#import "DevicesTableViewController.h"
#import "FnFunctionKeysTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "LogFileTextViewController.h"
#import "NotificationKeys.h"
#import "ProfilesTableViewController.h"
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
@property(weak) IBOutlet NSTableView* devicesTableView;
@property(weak) IBOutlet NSTableView* devicesExternalKeyboardTableView;
@property(weak) IBOutlet NSTableView* fnFunctionKeysTableView;
@property(weak) IBOutlet NSTableView* simpleModificationsTableView;
@property(weak) IBOutlet NSTextField* versionLabel;
@property(weak) IBOutlet NSPopUpButton* virtualHIDKeyboardTypePopupButton;
@property(weak) IBOutlet NSTextField* virtualHIDKeyboardCapsLockDelayMillisecondsText;
@property(weak) IBOutlet NSStepper* virtualHIDKeyboardCapsLockDelayMillisecondsStepper;
@property(weak) IBOutlet NSButton* checkForUpdateOnStartupButton;
@property(weak) IBOutlet ProfilesTableViewController* profilesTableViewController;
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
  [self setupVirtualHIDKeyboardTypePopUpButton];
  [self setupVirtualHIDKeyboardCapsLockDelayMilliseconds:nil];
  [self.profilesTableViewController setup];
  [self setupMiscTabControls];
  [self.logFileTextViewController monitor];

  @weakify(self);
  [[NSNotificationCenter defaultCenter] addObserverForName:kKarabinerKitConfigurationIsLoaded
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
                                                  @strongify(self);
                                                  if (!self) return;

                                                  [self setupVirtualHIDKeyboardTypePopUpButton];
                                                  [self setupVirtualHIDKeyboardCapsLockDelayMilliseconds:nil];
                                                  [self setupMiscTabControls];
                                                }];
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

- (void)setupVirtualHIDKeyboardTypePopUpButton {
  NSMenu* menu = [NSMenu new];

  {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"ANSI"
                                                  action:NULL
                                           keyEquivalent:@""];
    item.representedObject = @"ansi";
    [menu addItem:item];
  }
  {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"ISO"
                                                  action:NULL
                                           keyEquivalent:@""];
    item.representedObject = @"iso";
    [menu addItem:item];
  }
  {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"JIS"
                                                  action:NULL
                                           keyEquivalent:@""];
    item.representedObject = @"jis";
    [menu addItem:item];
  }

  self.virtualHIDKeyboardTypePopupButton.menu = menu;

  // ----------------------------------------
  // Select item

  NSString* keyboardType = @"ansi";
  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  if (coreConfigurationModel) {
    keyboardType = coreConfigurationModel.currentProfile.virtualHIDKeyboardType;
  }

  for (NSMenuItem* item in self.virtualHIDKeyboardTypePopupButton.itemArray) {
    if ([item.representedObject isEqualToString:keyboardType]) {
      [self.virtualHIDKeyboardTypePopupButton selectItem:item];
      break;
    }
  }
}

- (void)setupVirtualHIDKeyboardCapsLockDelayMilliseconds:(id)sender {
  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  if (coreConfigurationModel) {
    if (sender != self.virtualHIDKeyboardCapsLockDelayMillisecondsText) {
      self.virtualHIDKeyboardCapsLockDelayMillisecondsText.stringValue = @(coreConfigurationModel.currentProfile.virtualHIDKeyboardCapsLockDelayMilliseconds).stringValue;
    }
    if (sender != self.virtualHIDKeyboardCapsLockDelayMillisecondsStepper) {
      self.virtualHIDKeyboardCapsLockDelayMillisecondsStepper.integerValue = coreConfigurationModel.currentProfile.virtualHIDKeyboardCapsLockDelayMilliseconds;
    }
  }
}

- (IBAction)changeVirtualHIDKeyboardTYpe:(id)sender {
  NSMenuItem* selectedItem = self.virtualHIDKeyboardTypePopupButton.selectedItem;
  if (selectedItem) {
    KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];
    if (configurationManager) {
      KarabinerKitCoreConfigurationModel* coreConfigurationModel = configurationManager.coreConfigurationModel;
      if (coreConfigurationModel) {
        coreConfigurationModel.currentProfile.virtualHIDKeyboardType = selectedItem.representedObject;
        [configurationManager save];
      }
    }
  }
}

- (IBAction)changeVirtualHIDKeyboardCapsLockDelayMilliseconds:(NSControl*)sender {
  // If sender.stringValue is empty, set "0"
  if (sender.integerValue == 0) {
    sender.integerValue = 0;
  }

  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];
  if (configurationManager) {
    KarabinerKitCoreConfigurationModel* coreConfigurationModel = configurationManager.coreConfigurationModel;
    if (coreConfigurationModel) {
      coreConfigurationModel.currentProfile.virtualHIDKeyboardCapsLockDelayMilliseconds = sender.integerValue;
      [configurationManager save];
    }
  }

  [self setupVirtualHIDKeyboardCapsLockDelayMilliseconds:sender];
}

- (void)updateSystemPreferencesUIValues {
  self.keyboardFnStateButton.state = self.systemPreferencesManager.systemPreferencesModel.keyboardFnState ? NSOnState : NSOffState;
}

- (IBAction)updateSystemPreferencesValues:(id)sender {
  SystemPreferencesModel* model = self.systemPreferencesManager.systemPreferencesModel;

  if (sender == self.keyboardFnStateButton) {
    model.keyboardFnState = (self.keyboardFnStateButton.state == NSOnState);
  }

  [self updateSystemPreferencesUIValues];
  [self.systemPreferencesManager updateSystemPreferencesValues:model];
}

- (void)setupMiscTabControls {
  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];
  if (configurationManager) {
    KarabinerKitCoreConfigurationModel* coreConfigurationModel = configurationManager.coreConfigurationModel;
    if (coreConfigurationModel) {
      if (coreConfigurationModel.globalConfiguration.checkForUpdatesOnStartup) {
        self.checkForUpdateOnStartupButton.state = NSOnState;
      } else {
        self.checkForUpdateOnStartupButton.state = NSOffState;
      }

      if (coreConfigurationModel.globalConfiguration.showInMenubar) {
        // ...
      }
    }
  }
}

- (IBAction)changeMiscTabControls:(id)sender {
  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];
  if (configurationManager) {
    KarabinerKitCoreConfigurationModel* coreConfigurationModel = configurationManager.coreConfigurationModel;
    if (coreConfigurationModel) {
      coreConfigurationModel.globalConfiguration.checkForUpdatesOnStartup = (self.checkForUpdateOnStartupButton.state == NSOnState);

      [configurationManager save];
    }
  }
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
