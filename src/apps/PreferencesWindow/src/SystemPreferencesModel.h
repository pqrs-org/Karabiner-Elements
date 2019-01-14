// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn/libkrbn.h"

@interface SystemPreferencesModel : NSObject

@property BOOL keyboardFnState;

- (instancetype _Nonnull)initWithValues:(const struct libkrbn_system_preferences* _Nonnull)system_preferences;

@end
