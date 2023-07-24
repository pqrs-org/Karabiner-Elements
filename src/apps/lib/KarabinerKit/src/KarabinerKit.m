#import "KarabinerKit/KarabinerKit.h"
#import "libkrbn/libkrbn.h"

@implementation KarabinerKit

+ (void)endAllAttachedSheets:(NSWindow*)window {
  for (;;) {
    NSWindow* sheet = window.attachedSheet;
    if (!sheet) {
      break;
    }

    [self endAllAttachedSheets:sheet];
    [window endSheet:sheet];
  }
}

+ (BOOL)quitKarabiner:(bool)askForConfirmation {
  if (askForConfirmation) {
    NSAlert* alert = [NSAlert new];
    alert.messageText = @"Are you sure you want to quit Karabiner-Elements?";
    alert.informativeText = @"The changed key will be restored after Karabiner-Elements is quit.";
    [alert addButtonWithTitle:@"Quit"];
    [alert addButtonWithTitle:@"Cancel"];
    if ([alert runModal] == NSAlertFirstButtonReturn) {
      libkrbn_launchctl_manage_console_user_server(false);
      libkrbn_launchctl_manage_notification_window(false);
      return YES;
    }
    return NO;
  } else {
    libkrbn_launchctl_manage_console_user_server(false);
    libkrbn_launchctl_manage_notification_window(false);
    return YES;
  }
}

@end
