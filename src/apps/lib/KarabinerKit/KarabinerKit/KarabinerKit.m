#import "KarabinerKit.h"
#import "libkrbn.h"

@implementation KarabinerKit

+ (void)setup {
  // initialize managers
  [KarabinerKitConfigurationManager sharedManager];
  [KarabinerKitDeviceManager sharedManager];
}

+ (BOOL)quitKarabinerWithConfirmation {
  NSAlert* alert = [NSAlert new];
  alert.messageText = @"Are you sure you want to quit Karabiner-Elements?";
  alert.informativeText = @"The changed key will be restored after Karabiner-Elements is quit.";
  [alert addButtonWithTitle:@"Quit"];
  [alert addButtonWithTitle:@"Cancel"];
  if ([alert runModal] == NSAlertFirstButtonReturn) {
    libkrbn_launchctl_manage_console_user_server(false);
    return YES;
  }
  return NO;
}

@end
