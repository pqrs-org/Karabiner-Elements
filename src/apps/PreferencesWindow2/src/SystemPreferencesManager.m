#import "SystemPreferencesManager.h"
#import "NotificationKeys.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

@interface SystemPreferencesManager ()

@property(readwrite) SystemPreferencesModel* systemPreferencesModel;

@end

static void system_preferences_updated_callback(const struct libkrbn_system_preferences_properties* properties,
                                                void* refcon) {
  SystemPreferencesManager* manager = (__bridge SystemPreferencesManager*)(refcon);
  if (!manager) {
    return;
  }

  SystemPreferencesModel* model = [[SystemPreferencesModel alloc] initWithValues:properties];

  @weakify(manager);
  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(manager);
    if (!manager) {
      return;
    }

    manager.systemPreferencesModel = model;

    [[NSNotificationCenter defaultCenter] postNotificationName:kSystemPreferencesValuesAreUpdated object:nil];
  });
}

@implementation SystemPreferencesManager

- (void)setup {
  libkrbn_enable_system_preferences_monitor(system_preferences_updated_callback,
                                            (__bridge void*)(self));
}

- (void)dealloc {
  libkrbn_disable_system_preferences_monitor();
}

- (void)updateSystemPreferencesValues:(SystemPreferencesModel*)model {
  NSUserDefaults* userDefaults = [NSUserDefaults standardUserDefaults];
  NSDictionary<NSString*, id>* dictionary = [userDefaults persistentDomainForName:NSGlobalDomain];
  NSMutableDictionary<NSString*, id>* mutableDictionary = [NSMutableDictionary dictionaryWithDictionary:dictionary];

  mutableDictionary[@"com.apple.keyboard.fnState"] = @(model.useFkeysAsStandardFunctionKeys);

  [userDefaults setPersistentDomain:mutableDictionary forName:NSGlobalDomain];
}

@end
