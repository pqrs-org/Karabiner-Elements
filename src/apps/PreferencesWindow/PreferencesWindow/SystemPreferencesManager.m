#import "SystemPreferencesManager.h"
#import "NotificationKeys.h"
#import "libkrbn.h"

@interface SystemPreferencesManager ()

@property libkrbn_system_preferences_monitor* libkrbn_system_preferences_monitor;
@property(readwrite) SystemPreferencesModel* systemPreferencesModel;

- (void)updateSystemPreferencesModel:(const struct libkrbn_system_preferences_values* _Nonnull)system_preferences_values;

@end

static void system_preferences_updated_callback(const struct libkrbn_system_preferences_values* _Nonnull system_preferences_values,
                                                void* _Nullable refcon) {
  SystemPreferencesManager* manager = (__bridge SystemPreferencesManager*)(refcon);
  [manager updateSystemPreferencesModel:system_preferences_values];
  [[NSNotificationCenter defaultCenter] postNotificationName:kSystemPreferencesValuesAreUpdated object:nil];
}

@implementation SystemPreferencesManager

- (void)setup {
  libkrbn_system_preferences_monitor* p = NULL;
  if (libkrbn_system_preferences_monitor_initialize(&p, system_preferences_updated_callback, (__bridge void*)(self))) {
    return;
  }
  self.libkrbn_system_preferences_monitor = p;
}

- (void)dealloc {
  if (self.libkrbn_system_preferences_monitor) {
    libkrbn_system_preferences_monitor* p = self.libkrbn_system_preferences_monitor;
    libkrbn_system_preferences_monitor_terminate(&p);
  }
}

- (void)updateSystemPreferencesModel:(const struct libkrbn_system_preferences_values* _Nonnull)system_preferences_values {
  self.systemPreferencesModel = [[SystemPreferencesModel alloc] initWithValues:system_preferences_values];
}

- (void)updateSystemPreferencesValues:(SystemPreferencesModel*)model {
  NSUserDefaults* userDefaults = [NSUserDefaults standardUserDefaults];
  NSDictionary<NSString*, id>* dictionary = [userDefaults persistentDomainForName:NSGlobalDomain];
  NSMutableDictionary<NSString*, id>* mutableDictionary = [NSMutableDictionary dictionaryWithDictionary:dictionary];

  mutableDictionary[@"com.apple.keyboard.fnState"] = @(model.keyboardFnState);
  mutableDictionary[@"InitialKeyRepeat"] = @(libkrbn_system_preferences_convert_key_repeat_milliseconds_to_system_preferences_value(model.initialKeyRepeatMilliseconds));
  mutableDictionary[@"KeyRepeat"] = @(libkrbn_system_preferences_convert_key_repeat_milliseconds_to_system_preferences_value(model.keyRepeatMilliseconds));

  [userDefaults setPersistentDomain:mutableDictionary forName:NSGlobalDomain];
}

@end
