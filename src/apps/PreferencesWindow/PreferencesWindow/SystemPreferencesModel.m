#import "SystemPreferencesModel.h"

@implementation SystemPreferencesModel

- (instancetype)initWithValues:(const struct libkrbn_system_preferences_values* _Nonnull)values {
  self = [super init];

  if (self) {
    _keyboardFnState = values->keyboard_fn_state;
  }

  return self;
}

@end
