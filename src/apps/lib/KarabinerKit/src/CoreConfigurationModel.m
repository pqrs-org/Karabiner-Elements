#import "KarabinerKit/CoreConfigurationModel.h"
#import "KarabinerKit/KarabinerKit.h"

@interface KarabinerKitCoreConfigurationModel ()

@property libkrbn_core_configuration* libkrbnCoreConfiguration;

@end

@implementation KarabinerKitCoreConfigurationModel

- (instancetype)initWithInitializedCoreConfiguration:(libkrbn_core_configuration*)initializedCoreConfiguration {
  self = [super init];

  if (self) {
    _libkrbnCoreConfiguration = initializedCoreConfiguration;
  }

  return self;
}

- (void)dealloc {
  if (self.libkrbnCoreConfiguration) {
    libkrbn_core_configuration* p = self.libkrbnCoreConfiguration;
    if (p) {
      libkrbn_core_configuration_terminate(&p);
    }
  }
}

- (void)save {
  libkrbn_core_configuration_save(self.libkrbnCoreConfiguration);
}

- (BOOL)globalConfigurationCheckForUpdatesOnStartup {
  return libkrbn_core_configuration_get_global_configuration_check_for_updates_on_startup(self.libkrbnCoreConfiguration);
}

- (void)setGlobalConfigurationCheckForUpdatesOnStartup:(BOOL)value {
  libkrbn_core_configuration_set_global_configuration_check_for_updates_on_startup(self.libkrbnCoreConfiguration, value);
}

- (BOOL)globalConfigurationShowInMenuBar {
  return libkrbn_core_configuration_get_global_configuration_show_in_menu_bar(self.libkrbnCoreConfiguration);
}

- (void)setGlobalConfigurationShowInMenuBar:(BOOL)value {
  libkrbn_core_configuration_set_global_configuration_show_in_menu_bar(self.libkrbnCoreConfiguration, value);
}

- (BOOL)globalConfigurationShowProfileNameInMenuBar {
  return libkrbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar(self.libkrbnCoreConfiguration);
}

- (void)setGlobalConfigurationShowProfileNameInMenuBar:(BOOL)value {
  libkrbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(self.libkrbnCoreConfiguration, value);
}

- (NSUInteger)profilesCount {
  return libkrbn_core_configuration_get_profiles_size(self.libkrbnCoreConfiguration);
}

- (NSString*)profileNameAtIndex:(NSUInteger)index {
  const char* p = libkrbn_core_configuration_get_profile_name(self.libkrbnCoreConfiguration, index);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

- (void)setProfileNameAtIndex:(NSUInteger)index name:(NSString*)name {
  libkrbn_core_configuration_set_profile_name(self.libkrbnCoreConfiguration, index, [name UTF8String]);
}

- (BOOL)profileSelectedAtIndex:(NSUInteger)index {
  return libkrbn_core_configuration_get_profile_selected(self.libkrbnCoreConfiguration, index);
}

- (void)selectProfileAtIndex:(NSUInteger)index {
  libkrbn_core_configuration_select_profile(self.libkrbnCoreConfiguration, index);
}

- (NSString*)selectedProfileName {
  const char* p = libkrbn_core_configuration_get_selected_profile_name(self.libkrbnCoreConfiguration);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

@end
