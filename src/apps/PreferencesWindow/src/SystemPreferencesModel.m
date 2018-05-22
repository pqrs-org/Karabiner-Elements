#import "SystemPreferencesModel.h"

@implementation SystemPreferencesModel

- (instancetype)initWithValues:(const struct libkrbn_system_preferences* _Nonnull)system_preferences {
  self = [super init];

  if (self) {
    _keyboardFnState = system_preferences->keyboard_fn_state;
  }

  return self;
}

@end
