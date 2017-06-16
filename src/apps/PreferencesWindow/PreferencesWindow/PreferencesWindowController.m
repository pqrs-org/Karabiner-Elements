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
#import "libkrbn.h"
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
@property(weak) IBOutlet NSButton* systemDefaultProfileCopyButton;
@property(weak) IBOutlet NSTextField* systemDefaultProfileStateLabel;
@property(weak) IBOutlet NSButton* systemDefaultProfileRemoveButton;
@property(weak) IBOutlet NSButton* showInMenuBarButton;
@property(weak) IBOutlet NSButton* showProfileNameInMenuBarButton;
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
  [[NSNotificationCenter defaultCenter] addObserverForName:kSelectedProfileChanged
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
                                                  @strongify(self);
                                                  if (!self) return;

                                                  [self setupVirtualHIDKeyboardTypePopUpButton];
                                                  [self setupVirtualHIDKeyboardCapsLockDelayMilliseconds:nil];
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
  libkrbn_launchctl_manage_console_user_server(true);
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)show {
  [self.window makeKeyAndOrderFront:self];
  [NSApp activateIgnoringOtherApps:YES];
}

- (void)tabView:(NSTabView*)tabView didSelectTabViewItem:(NSTabViewItem*)tabViewItem {
  [self.logFileTextViewController updateTabLabel];
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

  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  NSString* keyboardType = coreConfigurationModel.selectedProfileVirtualHIDKeyboardKeyboardType;

  for (NSMenuItem* item in self.virtualHIDKeyboardTypePopupButton.itemArray) {
    if ([item.representedObject isEqualToString:keyboardType]) {
      [self.virtualHIDKeyboardTypePopupButton selectItem:item];
      break;
    }
  }
}

- (void)setupVirtualHIDKeyboardCapsLockDelayMilliseconds:(id)sender {
  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  NSInteger milliseconds = coreConfigurationModel.selectedProfileVirtualHIDKeyboardCapsLockDelayMilliseconds;

  if (sender != self.virtualHIDKeyboardCapsLockDelayMillisecondsText) {
    self.virtualHIDKeyboardCapsLockDelayMillisecondsText.stringValue = @(milliseconds).stringValue;
  }
  if (sender != self.virtualHIDKeyboardCapsLockDelayMillisecondsStepper) {
    self.virtualHIDKeyboardCapsLockDelayMillisecondsStepper.integerValue = milliseconds;
  }
}

- (IBAction)changeVirtualHIDKeyboardTYpe:(id)sender {
  NSMenuItem* selectedItem = self.virtualHIDKeyboardTypePopupButton.selectedItem;
  if (selectedItem) {
    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    coreConfigurationModel.selectedProfileVirtualHIDKeyboardKeyboardType = selectedItem.representedObject;
    [coreConfigurationModel save];
  }
}

- (IBAction)changeVirtualHIDKeyboardCapsLockDelayMilliseconds:(NSControl*)sender {
  // If sender.stringValue is empty, set "0"
  if (sender.integerValue == 0) {
    sender.integerValue = 0;
  }

  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  coreConfigurationModel.selectedProfileVirtualHIDKeyboardCapsLockDelayMilliseconds = sender.integerValue;
  [coreConfigurationModel save];

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
  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;

  if (coreConfigurationModel.globalConfigurationCheckForUpdatesOnStartup) {
    self.checkForUpdateOnStartupButton.state = NSOnState;
  } else {
    self.checkForUpdateOnStartupButton.state = NSOffState;
  }

  if (libkrbn_system_core_configuration_file_path_exists()) {
    self.systemDefaultProfileStateLabel.hidden = YES;
    self.systemDefaultProfileRemoveButton.hidden = NO;
  } else {
    self.systemDefaultProfileStateLabel.hidden = NO;
    self.systemDefaultProfileRemoveButton.hidden = YES;
  }

  if (coreConfigurationModel.globalConfigurationShowInMenuBar) {
    self.showInMenuBarButton.state = NSOnState;
  } else {
    self.showInMenuBarButton.state = NSOffState;
  }

  if (coreConfigurationModel.globalConfigurationShowProfileNameInMenuBar) {
    self.showProfileNameInMenuBarButton.state = NSOnState;
  } else {
    self.showProfileNameInMenuBarButton.state = NSOffState;
  }
}

- (IBAction)changeMiscTabControls:(id)sender {
  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;

  coreConfigurationModel.globalConfigurationCheckForUpdatesOnStartup = (self.checkForUpdateOnStartupButton.state == NSOnState);
  coreConfigurationModel.globalConfigurationShowInMenuBar = (self.showInMenuBarButton.state == NSOnState);
  coreConfigurationModel.globalConfigurationShowProfileNameInMenuBar = (self.showProfileNameInMenuBarButton.state == NSOnState);

  [coreConfigurationModel save];

  libkrbn_launch_menu();
}

- (IBAction)checkForUpdatesStableOnly:(id)sender {
  libkrbn_check_for_updates_stable_only();
}

- (IBAction)checkForUpdatesWithBetaVersion:(id)sender {
  libkrbn_check_for_updates_with_beta_version();
}

- (IBAction)systemDefaultProfileCopy:(id)sender {
  // Ensure karabiner.json exists before copy.
  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  [coreConfigurationModel save];

  NSString* path = @"/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/copy_current_profile_to_system_default_profile.applescript";
  [[[NSAppleScript alloc] initWithContentsOfURL:[NSURL fileURLWithPath:path] error:nil] executeAndReturnError:nil];
  [self setupMiscTabControls];
}

- (IBAction)systemDefaultProfileRemove:(id)sender {
  NSString* path = @"/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/remove_system_default_profile.applescript";
  [[[NSAppleScript alloc] initWithContentsOfURL:[NSURL fileURLWithPath:path] error:nil] executeAndReturnError:nil];
  [self setupMiscTabControls];
}

- (IBAction)launchUninstaller:(id)sender {
  NSString* path = @"/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/uninstaller.applescript";
  [[[NSAppleScript alloc] initWithContentsOfURL:[NSURL fileURLWithPath:path] error:nil] executeAndReturnError:nil];
}

- (IBAction)openURL:(id)sender {
  [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[sender title]]];
}

- (IBAction)quitWithConfirmation:(id)sender {
  [KarabinerKit quitKarabinerWithConfirmation];
}

@end
