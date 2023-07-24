// -*- mode: objective-c -*-

@import AppKit;
#import "libkrbn/libkrbn.h"

@interface KarabinerKit : NSObject

+ (void)setup;
+ (void)endAllAttachedSheets:(NSWindow*)window;

+ (void)observeConsoleUserServerIsDisabledNotification;

+ (void)relaunch;
+ (BOOL)quitKarabiner:(bool)askForConfirmation;

@end
