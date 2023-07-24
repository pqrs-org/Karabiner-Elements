// -*- mode: objective-c -*-

@import AppKit;
#import "libkrbn/libkrbn.h"

@interface KarabinerKit : NSObject

+ (void)endAllAttachedSheets:(NSWindow*)window;
+ (BOOL)quitKarabiner:(bool)askForConfirmation;

@end
