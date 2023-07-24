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

+ (void)observeConsoleUserServerIsDisabledNotification {
  NSString* name = [NSString stringWithUTF8String:libkrbn_get_distributed_notification_console_user_server_is_disabled()];
  NSString* object = [NSString stringWithUTF8String:libkrbn_get_distributed_notification_observed_object()];

  [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                      selector:@selector(consoleUserServerIsDisabledCallback)
                                                          name:name
                                                        object:object
                                            suspensionBehavior:NSNotificationSuspensionBehaviorDeliverImmediately];
}

+ (void)consoleUserServerIsDisabledCallback {
  [NSApp terminate:nil];
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
