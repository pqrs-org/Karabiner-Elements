#import "SystemPreferencesModel.h"

@implementation SystemPreferencesModel

- (instancetype)initWithValues:(const struct libkrbn_system_preferences_values* _Nonnull)values {
  self = [super init];

  if (self) {
    _keyboardFnState = values->keyboard_fn_state;
    _initialKeyRepeatMilliseconds = values->initial_key_repeat_milliseconds;
    _keyRepeatMilliseconds = values->key_repeat_milliseconds;
  }

  return self;
}

@end
