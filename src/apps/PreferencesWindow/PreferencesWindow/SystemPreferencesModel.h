// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn.h"

@interface SystemPreferencesModel : NSObject

@property BOOL keyboardFnState;

- (instancetype _Nonnull)initWithValues:(const struct libkrbn_system_preferences_values* _Nonnull)values;

@end
