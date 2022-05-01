#import "PreferencesWindowController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "Karabiner_Elements-Swift.h"
#import "NotificationKeys.h"
#import "SystemPreferencesManager.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

@interface PreferencesWindowController ()

@property(weak) IBOutlet NSButton* useFkeysAsStandardFunctionKeysButton;
@property(weak) IBOutlet SystemPreferencesManager* systemPreferencesManager;

@end

@implementation PreferencesWindowController

- (void)setup {
  // ----------------------------------------
  // Update UI values

  [self updateSystemPreferencesUIValues];

  // ----------------------------------------
  libkrbn_launchctl_manage_session_monitor();
  libkrbn_launchctl_manage_console_user_server(true);
  // Do not manage grabber_agent and observer_agent because they are designed to run only once.
}

- (void)updateSystemPreferencesUIValues {
  self.useFkeysAsStandardFunctionKeysButton.state = self.systemPreferencesManager.systemPreferencesModel.useFkeysAsStandardFunctionKeys ? NSControlStateValueOn : NSControlStateValueOff;
}

- (IBAction)updateSystemPreferencesValues:(id)sender {
  SystemPreferencesModel* model = self.systemPreferencesManager.systemPreferencesModel;

  if (sender == self.useFkeysAsStandardFunctionKeysButton) {
    model.useFkeysAsStandardFunctionKeys = (self.useFkeysAsStandardFunctionKeysButton.state == NSControlStateValueOn);
  }

  [self updateSystemPreferencesUIValues];
  [self.systemPreferencesManager updateSystemPreferencesValues:model];
}

@end
