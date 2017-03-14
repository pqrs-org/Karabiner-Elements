#import "CoreConfigurationModel.h"

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

- (BOOL)save {
  return libkrbn_core_configuration_save(self.libkrbnCoreConfiguration);
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

- (void)addProfile {
  libkrbn_core_configuration_push_back_profile(self.libkrbnCoreConfiguration);
}

- (void)removeProfileAtIndex:(NSUInteger)index {
  libkrbn_core_configuration_erase_profile(self.libkrbnCoreConfiguration, index);
}

- (NSUInteger)selectedProfileSimpleModificationsCount {
  return libkrbn_core_configuration_get_selected_profile_simple_modifications_size(self.libkrbnCoreConfiguration);
}

- (NSString*)selectedProfileSimpleModificationFirstAtIndex:(NSUInteger)index {
  const char* p = libkrbn_core_configuration_get_selected_profile_simple_modification_first(self.libkrbnCoreConfiguration, index);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

- (NSString*)selectedProfileSimpleModificationSecondAtIndex:(NSUInteger)index {
  const char* p = libkrbn_core_configuration_get_selected_profile_simple_modification_second(self.libkrbnCoreConfiguration, index);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

- (void)setSelectedProfileSimpleModificationAtIndex:(NSUInteger)index from:(NSString*)from to:(NSString*)to {
  libkrbn_core_configuration_replace_selected_profile_simple_modification(self.libkrbnCoreConfiguration, index, [from UTF8String], [to UTF8String]);
}

- (void)addSimpleModificationToSelectedProfile {
  libkrbn_core_configuration_push_back_selected_profile_simple_modification(self.libkrbnCoreConfiguration);
}

- (void)removeSelectedProfileSimpleModificationAtIndex:(NSUInteger)index {
  libkrbn_core_configuration_erase_selected_profile_simple_modification(self.libkrbnCoreConfiguration, index);
}

- (NSUInteger)selectedProfileFnFunctionKeysCount {
  return libkrbn_core_configuration_get_selected_profile_fn_function_keys_size(self.libkrbnCoreConfiguration);
}

- (NSString*)selectedProfileFnFunctionKeyFirstAtIndex:(NSUInteger)index {
  const char* p = libkrbn_core_configuration_get_selected_profile_fn_function_key_first(self.libkrbnCoreConfiguration, index);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

- (NSString*)selectedProfileFnFunctionKeySecondAtIndex:(NSUInteger)index {
  const char* p = libkrbn_core_configuration_get_selected_profile_fn_function_key_second(self.libkrbnCoreConfiguration, index);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

- (void)setSelectedProfileFnFunctionKey:(NSString*)from to:(NSString*)to {
  libkrbn_core_configuration_replace_selected_profile_fn_function_key(self.libkrbnCoreConfiguration, [from UTF8String], [to UTF8String]);
}

- (BOOL)selectedProfileDeviceIgnore:(NSUInteger)vendorId
                          productId:(NSUInteger)productId
                         isKeyboard:(BOOL)isKeyboard
                   isPointingDevice:(BOOL)isPointingDevice {
  return libkrbn_core_configuration_get_selected_profile_device_ignore(self.libkrbnCoreConfiguration,
                                                                       (uint32_t)(vendorId),
                                                                       (uint32_t)(productId),
                                                                       isKeyboard,
                                                                       isPointingDevice);
}

- (void)setSelectedProfileDeviceIgnore:(NSUInteger)vendorId
                             productId:(NSUInteger)productId
                            isKeyboard:(BOOL)isKeyboard
                      isPointingDevice:(BOOL)isPointingDevice
                                 value:(BOOL)value {
  libkrbn_core_configuration_set_selected_profile_device_ignore(self.libkrbnCoreConfiguration,
                                                                (uint32_t)(vendorId),
                                                                (uint32_t)(productId),
                                                                isKeyboard,
                                                                isPointingDevice,
                                                                value);
}

- (BOOL)selectedProfileDeviceDisableBuiltInKeyboardIfExists:(NSUInteger)vendorId
                                                  productId:(NSUInteger)productId
                                                 isKeyboard:(BOOL)isKeyboard
                                           isPointingDevice:(BOOL)isPointingDevice {
  return libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(self.libkrbnCoreConfiguration,
                                                                                                    (uint32_t)(vendorId),
                                                                                                    (uint32_t)(productId),
                                                                                                    isKeyboard,
                                                                                                    isPointingDevice);
}

- (void)setSelectedProfileDeviceDisableBuiltInKeyboardIfExists:(NSUInteger)vendorId
                                                     productId:(NSUInteger)productId
                                                    isKeyboard:(BOOL)isKeyboard
                                              isPointingDevice:(BOOL)isPointingDevice
                                                         value:(BOOL)value {
  libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(self.libkrbnCoreConfiguration,
                                                                                             (uint32_t)(vendorId),
                                                                                             (uint32_t)(productId),
                                                                                             isKeyboard,
                                                                                             isPointingDevice,
                                                                                             value);
}

- (NSString*)selectedProfileVirtualHIDKeyboardKeyboardType {
  const char* p = libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_keyboard_type(self.libkrbnCoreConfiguration);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

- (void)setSelectedProfileVirtualHIDKeyboardKeyboardType:(NSString*)value {
  libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_keyboard_type(self.libkrbnCoreConfiguration, [value UTF8String]);
}

- (NSInteger)selectedProfileVirtualHIDKeyboardCapsLockDelayMilliseconds {
  return libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_caps_lock_delay_milliseconds(self.libkrbnCoreConfiguration);
}

- (void)setSelectedProfileVirtualHIDKeyboardCapsLockDelayMilliseconds:(NSInteger)value {
  libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_caps_lock_delay_milliseconds(self.libkrbnCoreConfiguration, (uint32_t)(value));
}

@end
