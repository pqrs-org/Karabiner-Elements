#import "CoreConfigurationModel.h"
#import "JsonUtility.h"
#import "libkrbn.h"

@interface KarabinerKitCoreConfigurationModel ()

@property libkrbn_core_configuration* libkrbn_core_configuration;

@end

@implementation KarabinerKitCoreConfigurationModel

- (instancetype)init {
  self = [super init];

  if (self) {
    libkrbn_core_configuration* p = NULL;
    if (libkrbn_core_configuration_initialize(&p, libkrbn_get_user_core_configuration_file_path())) {
      _libkrbn_core_configuration = p;
    }
  }

  return self;
}

- (BOOL)isLoaded {
  return libkrbn_core_configuration_is_loaded(self.libkrbn_core_configuration);
}

- (BOOL)save {
  return libkrbn_core_configuration_save(self.libkrbn_core_configuration);
}

- (BOOL)globalConfigurationCheckForUpdatesOnStartup {
  return libkrbn_core_configuration_get_global_configuration_check_for_updates_on_startup(self.libkrbn_core_configuration);
}

- (void)setGlobalConfigurationCheckForUpdatesOnStartup:(BOOL)value {
  libkrbn_core_configuration_set_global_configuration_check_for_updates_on_startup(self.libkrbn_core_configuration, value);
}

- (BOOL)globalConfigurationShowInMenuBar {
  return libkrbn_core_configuration_get_global_configuration_show_in_menu_bar(self.libkrbn_core_configuration);
}

- (void)setGlobalConfigurationShowInMenuBar:(BOOL)value {
  libkrbn_core_configuration_set_global_configuration_show_in_menu_bar(self.libkrbn_core_configuration, value);
}

- (BOOL)globalConfigurationShowProfileNameInMenuBar {
  return libkrbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar(self.libkrbn_core_configuration);
}

- (void)setGlobalConfigurationShowProfileNameInMenuBar:(BOOL)value {
  libkrbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(self.libkrbn_core_configuration, value);
}

- (NSUInteger)profilesCount {
  return libkrbn_core_configuration_get_profiles_size(self.libkrbn_core_configuration);
}

- (NSString*)profileNameAtIndex:(NSUInteger)index {
  const char* p = libkrbn_core_configuration_get_profile_name(self.libkrbn_core_configuration, index);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

- (void)setProfileNameAtIndex:(NSUInteger)index name:(NSString*)name {
  libkrbn_core_configuration_set_profile_name(self.libkrbn_core_configuration, index, [name UTF8String]);
}

- (BOOL)profileSelectedAtIndex:(NSUInteger)index {
  return libkrbn_core_configuration_get_profile_selected(self.libkrbn_core_configuration, index);
}

- (void)selectProfileAtIndex:(NSUInteger)index {
  libkrbn_core_configuration_select_profile(self.libkrbn_core_configuration, index);
}

- (NSString*)selectedProfileName {
  const char* p = libkrbn_core_configuration_get_selected_profile_name(self.libkrbn_core_configuration);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

- (void)addProfile {
  libkrbn_core_configuration_push_back_profile(self.libkrbn_core_configuration);
}

- (void)removeProfileAtIndex:(NSUInteger)index {
  libkrbn_core_configuration_erase_profile(self.libkrbn_core_configuration, index);
}

- (NSUInteger)selectedProfileSimpleModificationsCount {
  return libkrbn_core_configuration_get_selected_profile_simple_modifications_size(self.libkrbn_core_configuration);
}

- (NSString*)selectedProfileSimpleModificationFirstAtIndex:(NSUInteger)index {
  const char* p = libkrbn_core_configuration_get_selected_profile_simple_modification_first(self.libkrbn_core_configuration, index);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

- (NSString*)selectedProfileSimpleModificationSecondAtIndex:(NSUInteger)index {
  const char* p = libkrbn_core_configuration_get_selected_profile_simple_modification_second(self.libkrbn_core_configuration, index);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

- (void)setSelectedProfileSimpleModificationAtIndex:(NSUInteger)index from:(NSString*)from to:(NSString*)to {
  libkrbn_core_configuration_replace_selected_profile_simple_modification(self.libkrbn_core_configuration, index, [from UTF8String], [to UTF8String]);
}

- (void)addSimpleModificationToSelectedProfile {
  libkrbn_core_configuration_push_back_selected_profile_simple_modification(self.libkrbn_core_configuration);
}

- (void)removeSelectedProfileSimpleModificationAtIndex:(NSUInteger)index {
  libkrbn_core_configuration_erase_selected_profile_simple_modification(self.libkrbn_core_configuration, index);
}

- (NSUInteger)selectedProfileFnFunctionKeysCount {
  return libkrbn_core_configuration_get_selected_profile_fn_function_keys_size(self.libkrbn_core_configuration);
}

- (NSString*)selectedProfileFnFunctionKeyFirstAtIndex:(NSUInteger)index {
  const char* p = libkrbn_core_configuration_get_selected_profile_fn_function_key_first(self.libkrbn_core_configuration, index);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

- (NSString*)selectedProfileFnFunctionKeySecondAtIndex:(NSUInteger)index {
  const char* p = libkrbn_core_configuration_get_selected_profile_fn_function_key_second(self.libkrbn_core_configuration, index);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

- (void)setSelectedProfileFnFunctionKey:(NSString*)from to:(NSString*)to {
  libkrbn_core_configuration_replace_selected_profile_fn_function_key(self.libkrbn_core_configuration, [from UTF8String], [to UTF8String]);
}

- (BOOL)selectedProfileDeviceIgnore:(NSUInteger)vendorId
                          productId:(NSUInteger)productId
                         isKeyboard:(BOOL)isKeyboard
                   isPointingDevice:(BOOL)isPointingDevice {
  return libkrbn_core_configuration_get_selected_profile_device_ignore(self.libkrbn_core_configuration,
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
  libkrbn_core_configuration_set_selected_profile_device_ignore(self.libkrbn_core_configuration,
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
  return libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(self.libkrbn_core_configuration,
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
  libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(self.libkrbn_core_configuration,
                                                                                             (uint32_t)(vendorId),
                                                                                             (uint32_t)(productId),
                                                                                             isKeyboard,
                                                                                             isPointingDevice,
                                                                                             value);
}

- (NSString*)selectedProfileVirtualHIDKeyboardKeyboardType {
  const char* p = libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_keyboard_type(self.libkrbn_core_configuration);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

- (void)setSelectedProfileVirtualHIDKeyboardKeyboardType:(NSString*)value {
  libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_keyboard_type(self.libkrbn_core_configuration, [value UTF8String]);
}

- (NSInteger)selectedProfileVirtualHIDKeyboardCapsLockDelayMilliseconds {
  return libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_caps_lock_delay_milliseconds(self.libkrbn_core_configuration);
}

- (void)setSelectedProfileVirtualHIDKeyboardCapsLockDelayMilliseconds:(NSInteger)value {
  libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_caps_lock_delay_milliseconds(self.libkrbn_core_configuration, (uint32_t)(value));
}

@end
