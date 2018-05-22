#import "SystemPreferencesManager.h"
#import "NotificationKeys.h"
#import "libkrbn.h"

@interface SystemPreferencesManager ()

@property libkrbn_configuration_monitor* libkrbn_configuration_monitor;
@property libkrbn_system_preferences_monitor* libkrbn_system_preferences_monitor;
@property(readwrite) SystemPreferencesModel* systemPreferencesModel;

- (void)updateSystemPreferencesModel:(const struct libkrbn_system_preferences* _Nonnull)system_preferences;

@end

static void configuration_file_updated_callback(libkrbn_core_configuration* initializedCoreConfiguration,
                                                void* refcon) {
}

static void system_preferences_updated_callback(const struct libkrbn_system_preferences* _Nonnull system_preferences,
                                                void* _Nullable refcon) {
  SystemPreferencesManager* manager = (__bridge SystemPreferencesManager*)(refcon);
  [manager updateSystemPreferencesModel:system_preferences];
  [[NSNotificationCenter defaultCenter] postNotificationName:kSystemPreferencesValuesAreUpdated object:nil];
}

@implementation SystemPreferencesManager

- (void)setup {
  libkrbn_configuration_monitor* configuration_monitor = NULL;
  if (libkrbn_configuration_monitor_initialize(&configuration_monitor,
                                               configuration_file_updated_callback,
                                               (__bridge void*)(self))) {
    self.libkrbn_configuration_monitor = configuration_monitor;
  }

  libkrbn_system_preferences_monitor* system_preferences_monitor = NULL;
  if (libkrbn_system_preferences_monitor_initialize(&system_preferences_monitor,
                                                    system_preferences_updated_callback,
                                                    (__bridge void*)(self),
                                                    configuration_monitor)) {
    self.libkrbn_system_preferences_monitor = system_preferences_monitor;
  }
}

- (void)dealloc {
  if (self.libkrbn_system_preferences_monitor) {
    libkrbn_system_preferences_monitor* p = self.libkrbn_system_preferences_monitor;
    libkrbn_system_preferences_monitor_terminate(&p);
  }
  if (self.libkrbn_configuration_monitor) {
    libkrbn_configuration_monitor* p = self.libkrbn_configuration_monitor;
    libkrbn_configuration_monitor_terminate(&p);
  }
}

- (void)updateSystemPreferencesModel:(const struct libkrbn_system_preferences* _Nonnull)system_preferences {
  self.systemPreferencesModel = [[SystemPreferencesModel alloc] initWithValues:system_preferences];
}

- (void)updateSystemPreferencesValues:(SystemPreferencesModel*)model {
  NSUserDefaults* userDefaults = [NSUserDefaults standardUserDefaults];
  NSDictionary<NSString*, id>* dictionary = [userDefaults persistentDomainForName:NSGlobalDomain];
  NSMutableDictionary<NSString*, id>* mutableDictionary = [NSMutableDictionary dictionaryWithDictionary:dictionary];

  mutableDictionary[@"com.apple.keyboard.fnState"] = @(model.keyboardFnState);

  [userDefaults setPersistentDomain:mutableDictionary forName:NSGlobalDomain];
}

@end
