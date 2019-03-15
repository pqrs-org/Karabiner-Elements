// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn/libkrbn.h"

@interface SystemPreferencesModel : NSObject

@property BOOL useFkeysAsStandardFunctionKeys;

- (instancetype)initWithValues:(const struct libkrbn_system_preferences_properties*)system_preferences;

@end
