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

- (void)addProfile {
  libkrbn_core_configuration_push_back_profile(self.libkrbnCoreConfiguration);
}

- (void)removeProfileAtIndex:(NSUInteger)index {
  libkrbn_core_configuration_erase_profile(self.libkrbnCoreConfiguration, index);
}

- (NSInteger)selectedProfileParametersDelayMillisecondsBeforeOpenDevice {
  return libkrbn_core_configuration_get_selected_profile_parameters_delay_milliseconds_before_open_device(
      self.libkrbnCoreConfiguration);
}

- (void)setSelectedProfileParametersDelayMillisecondsBeforeOpenDevice:(NSInteger)value {
  libkrbn_core_configuration_set_selected_profile_parameters_delay_milliseconds_before_open_device(
      self.libkrbnCoreConfiguration, (int)(value));
}

- (libkrbn_device_identifiers*)deviceIdentifiersByIndex:(NSInteger)connectedDeviceIndex
                                      deviceIdentifiers:(libkrbn_device_identifiers*)deviceIdentifiers {
  if (deviceIdentifiers) {
    KarabinerKitConnectedDevices* connectedDevices = [KarabinerKitDeviceManager sharedManager].connectedDevices;
    if (0 <= connectedDeviceIndex && connectedDeviceIndex < (NSInteger)(connectedDevices.devicesCount)) {
      *deviceIdentifiers = [connectedDevices deviceIdentifiersAtIndex:connectedDeviceIndex];
      return deviceIdentifiers;
    }
  }
  return nil;
}

- (NSUInteger)selectedProfileSimpleModificationsCount:(NSInteger)connectedDeviceIndex {
  libkrbn_device_identifiers deviceIdentifiers;
  return libkrbn_core_configuration_get_selected_profile_simple_modifications_size(self.libkrbnCoreConfiguration,
                                                                                   [self deviceIdentifiersByIndex:connectedDeviceIndex
                                                                                                deviceIdentifiers:&deviceIdentifiers]);
}

- (NSString*)selectedProfileSimpleModificationFromJsonStringAtIndex:(NSUInteger)index
                                               connectedDeviceIndex:(NSInteger)connectedDeviceIndex {
  libkrbn_device_identifiers deviceIdentifiers;
  const char* p = libkrbn_core_configuration_get_selected_profile_simple_modification_from_json_string(
      self.libkrbnCoreConfiguration,
      index,
      [self deviceIdentifiersByIndex:connectedDeviceIndex
                   deviceIdentifiers:&deviceIdentifiers]);
  return p ? [NSString stringWithUTF8String:p] : @"";
}

- (NSString*)selectedProfileSimpleModificationToJsonStringAtIndex:(NSUInteger)index
                                             connectedDeviceIndex:(NSInteger)connectedDeviceIndex {
  libkrbn_device_identifiers deviceIdentifiers;
  const char* p = libkrbn_core_configuration_get_selected_profile_simple_modification_to_json_string(
      self.libkrbnCoreConfiguration,
      index,
      [self deviceIdentifiersByIndex:connectedDeviceIndex
                   deviceIdentifiers:&deviceIdentifiers]);
  return p ? [NSString stringWithUTF8String:p] : @"";
}

- (void)setSelectedProfileSimpleModificationAtIndex:(NSUInteger)index
                                               from:(NSString*)fromJsonString
                                                 to:(NSString*)toJsonString
                               connectedDeviceIndex:(NSInteger)connectedDeviceIndex {
  libkrbn_device_identifiers deviceIdentifiers;

  libkrbn_core_configuration_replace_selected_profile_simple_modification(self.libkrbnCoreConfiguration,
                                                                          index,
                                                                          fromJsonString.UTF8String,
                                                                          toJsonString.UTF8String,
                                                                          [self deviceIdentifiersByIndex:connectedDeviceIndex
                                                                                       deviceIdentifiers:&deviceIdentifiers]);
}

- (void)addSimpleModificationToSelectedProfile:(NSInteger)connectedDeviceIndex {
  libkrbn_device_identifiers deviceIdentifiers;
  libkrbn_core_configuration_push_back_selected_profile_simple_modification(self.libkrbnCoreConfiguration,
                                                                            [self deviceIdentifiersByIndex:connectedDeviceIndex
                                                                                         deviceIdentifiers:&deviceIdentifiers]);
}

- (void)removeSelectedProfileSimpleModificationAtIndex:(NSUInteger)index connectedDeviceIndex:(NSInteger)connectedDeviceIndex {
  libkrbn_device_identifiers deviceIdentifiers;
  libkrbn_core_configuration_erase_selected_profile_simple_modification(self.libkrbnCoreConfiguration,
                                                                        index,
                                                                        [self deviceIdentifiersByIndex:connectedDeviceIndex
                                                                                     deviceIdentifiers:&deviceIdentifiers]);
}

- (NSUInteger)selectedProfileFnFunctionKeysCount:(NSInteger)connectedDeviceIndex {
  libkrbn_device_identifiers deviceIdentifiers;
  return libkrbn_core_configuration_get_selected_profile_fn_function_keys_size(self.libkrbnCoreConfiguration,
                                                                               [self deviceIdentifiersByIndex:connectedDeviceIndex
                                                                                            deviceIdentifiers:&deviceIdentifiers]);
}

- (NSString*)selectedProfileFnFunctionKeyFromJsonStringAtIndex:(NSUInteger)index
                                          connectedDeviceIndex:(NSInteger)connectedDeviceIndex {
  libkrbn_device_identifiers deviceIdentifiers;
  const char* p = libkrbn_core_configuration_get_selected_profile_fn_function_key_from_json_string(
      self.libkrbnCoreConfiguration,
      index,
      [self deviceIdentifiersByIndex:connectedDeviceIndex
                   deviceIdentifiers:&deviceIdentifiers]);
  return p ? [NSString stringWithUTF8String:p] : @"";
}

- (NSString*)selectedProfileFnFunctionKeyToJsonStringAtIndex:(NSUInteger)index
                                        connectedDeviceIndex:(NSInteger)connectedDeviceIndex {
  libkrbn_device_identifiers deviceIdentifiers;
  const char* p = libkrbn_core_configuration_get_selected_profile_fn_function_key_to_json_string(
      self.libkrbnCoreConfiguration,
      index,
      [self deviceIdentifiersByIndex:connectedDeviceIndex
                   deviceIdentifiers:&deviceIdentifiers]);
  return p ? [NSString stringWithUTF8String:p] : @"";
}

- (void)setSelectedProfileFnFunctionKey:(NSString*)from
                                     to:(NSString*)to
                   connectedDeviceIndex:(NSInteger)connectedDeviceIndex {
  libkrbn_device_identifiers deviceIdentifiers;
  libkrbn_core_configuration_replace_selected_profile_fn_function_key(self.libkrbnCoreConfiguration,
                                                                      from.UTF8String,
                                                                      to.UTF8String,
                                                                      [self deviceIdentifiersByIndex:connectedDeviceIndex
                                                                                   deviceIdentifiers:&deviceIdentifiers]);
}

- (NSUInteger)selectedProfileComplexModificationsRulesCount {
  return libkrbn_core_configuration_get_selected_profile_complex_modifications_rules_size(self.libkrbnCoreConfiguration);
}

- (NSString*)selectedProfileComplexModificationsRuleDescription:(NSUInteger)index {
  const char* p = libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_description(self.libkrbnCoreConfiguration, index);
  if (p) {
    return [NSString stringWithUTF8String:p];
  }
  return @"";
}

- (void)removeSelectedProfileComplexModificationsRule:(NSUInteger)index {
  libkrbn_core_configuration_erase_selected_profile_complex_modifications_rule(self.libkrbnCoreConfiguration, index);
}

- (void)swapSelectedProfileComplexModificationsRules:(NSUInteger)index1 index2:(NSUInteger)index2 {
  libkrbn_core_configuration_swap_selected_profile_complex_modifications_rules(self.libkrbnCoreConfiguration, index1, index2);
}

- (int)getSelectedProfileComplexModificationsParameter:(NSString*)name {
  return libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(self.libkrbnCoreConfiguration, [name UTF8String]);
}

- (void)setSelectedProfileComplexModificationsParameter:(NSString*)name value:(int)value {
  libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter(self.libkrbnCoreConfiguration, [name UTF8String], value);
}

- (BOOL)selectedProfileDeviceIgnore:(const libkrbn_device_identifiers*)deviceIdentifiers {
  return libkrbn_core_configuration_get_selected_profile_device_ignore(self.libkrbnCoreConfiguration,
                                                                       deviceIdentifiers);
}

- (void)setSelectedProfileDeviceIgnore:(const libkrbn_device_identifiers*)deviceIdentifiers
                                 value:(BOOL)value {
  libkrbn_core_configuration_set_selected_profile_device_ignore(self.libkrbnCoreConfiguration,
                                                                deviceIdentifiers,
                                                                value);
}

- (BOOL)selectedProfileDeviceManipulateCapsLockLed:(const libkrbn_device_identifiers*)deviceIdentifiers {
  return libkrbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led(self.libkrbnCoreConfiguration,
                                                                                         deviceIdentifiers);
}

- (void)setSelectedProfileDeviceManipulateCapsLockLed:(const libkrbn_device_identifiers*)deviceIdentifiers
                                                value:(BOOL)value {
  libkrbn_core_configuration_set_selected_profile_device_manipulate_caps_lock_led(self.libkrbnCoreConfiguration,
                                                                                  deviceIdentifiers,
                                                                                  value);
}

- (BOOL)selectedProfileDeviceDisableBuiltInKeyboardIfExists:(const libkrbn_device_identifiers*)deviceIdentifiers {
  return libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(self.libkrbnCoreConfiguration,
                                                                                                    deviceIdentifiers);
}

- (void)setSelectedProfileDeviceDisableBuiltInKeyboardIfExists:(const libkrbn_device_identifiers*)deviceIdentifiers
                                                         value:(BOOL)value {
  libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(self.libkrbnCoreConfiguration,
                                                                                             deviceIdentifiers,
                                                                                             value);
}

- (NSInteger)selectedProfileVirtualHIDKeyboardCountryCode {
  return libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_country_code(self.libkrbnCoreConfiguration);
}

- (void)setSelectedProfileVirtualHIDKeyboardCountryCode:(NSInteger)value {
  libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_country_code(self.libkrbnCoreConfiguration, (uint8_t)(value));
}

- (NSInteger)selectedProfileVirtualHIDKeyboardMouseKeyXYScale {
  return libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(self.libkrbnCoreConfiguration);
}

- (void)setSelectedProfileVirtualHIDKeyboardMouseKeyXYScale:(NSInteger)value {
  libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(self.libkrbnCoreConfiguration, (int)(value));
}

- (NSInteger)selectedProfileVirtualHIDKeyboardIndicateStickyModifierKeysState {
  return libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(self.libkrbnCoreConfiguration);
}

- (void)setSelectedProfileVirtualHIDKeyboardIndicateStickyModifierKeysState:(NSInteger)value {
  libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(self.libkrbnCoreConfiguration, (int)(value));
}

@end
