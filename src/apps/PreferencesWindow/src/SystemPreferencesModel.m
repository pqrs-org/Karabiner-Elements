#import "SystemPreferencesModel.h"

@implementation SystemPreferencesModel

- (instancetype)initWithValues:(const struct libkrbn_system_preferences_properties*)properties {
  self = [super init];

  if (self) {
    if (properties) {
      _useFkeysAsStandardFunctionKeys = properties->use_fkeys_as_standard_function_keys;
    }
  }

  return self;
}

@end
