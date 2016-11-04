// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn.h"

@interface SystemPreferencesModel : NSObject

@property BOOL keyboardFnState;
@property uint32_t initialKeyRepeatMilliseconds;
@property uint32_t keyRepeatMilliseconds;

- (instancetype _Nonnull)initWithValues:(const struct libkrbn_system_preferences_values* _Nonnull)values;

@end
