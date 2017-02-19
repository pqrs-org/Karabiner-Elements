#import "CoreConfigurationModel.h"
#import "JsonUtility.h"
#import "libkrbn.h"

@implementation KarabinerKitGlobalConfiguration

// jsonObject:
//   {
//       "check_for_updates_on_startup": true,
//       "show_in_menu_bar": true
//   }

- (instancetype)initWithJsonObject:(NSDictionary*)jsonObject {
  self = [super init];

  if (self) {
    _checkForUpdatesOnStartup = YES;
    _showInMenuBar = YES;
    _showProfileNameInMenuBar = NO;

    if ([jsonObject isKindOfClass:[NSDictionary class]]) {
      {
        NSNumber* value = jsonObject[@"check_for_updates_on_startup"];
        if (value) {
          _checkForUpdatesOnStartup = [value boolValue];
        }
      }
      {
        NSNumber* value = jsonObject[@"show_in_menu_bar"];
        if (value) {
          _showInMenuBar = [value boolValue];
        }
      }
      {
        NSNumber* value = jsonObject[@"show_profile_name_in_menu_bar"];
        if (value) {
          _showProfileNameInMenuBar = [value boolValue];
        }
      }
    }
  }

  return self;
}

@end

@interface KarabinerKitDeviceConfiguration ()

@property(readwrite) KarabinerKitDeviceIdentifiers* deviceIdentifiers;

// jsonObject:
//   {
//       "identifiers": {
//           "vendor_id": 1133,
//           "product_id": 50475,
//           "is_keyboard": true,
//           "is_pointing_device": false
//       },
//       "ignore": false
//   },

- (instancetype)initWithJsonObject:(NSDictionary*)jsonObject;

@end

@implementation KarabinerKitDeviceConfiguration

- (instancetype)initWithJsonObject:(NSDictionary*)jsonObject {
  self = [super init];

  if (self) {
    if ([jsonObject isKindOfClass:[NSDictionary class]]) {
      NSDictionary* identifiers = jsonObject[@"identifiers"];
      if (identifiers) {
        _deviceIdentifiers = [[KarabinerKitDeviceIdentifiers alloc] initWithDictionary:identifiers];
      }

      _ignore = [jsonObject[@"ignore"] boolValue];
      _disableBuiltInKeyboardIfExists = [jsonObject[@"disable_built_in_keyboard_if_exists"] boolValue];
    }
  }

  return self;
}

@end

@interface KarabinerKitConfigurationProfile ()

@property(copy) NSDictionary* originalJsonObject;
@property(copy) NSArray<NSDictionary*>* simpleModifications;
@property(copy) NSArray<NSDictionary*>* fnFunctionKeys;
@property(copy) NSArray<KarabinerKitDeviceConfiguration*>* devices;

@end

@implementation KarabinerKitConfigurationProfile

- (instancetype)initWithJsonObject:(NSDictionary*)jsonObject {
  self = [super init];

  if (self) {
    if ([jsonObject isKindOfClass:[NSDictionary class]]) {
      // Save passed jsonObject in order to keep unknown keys.
      _originalJsonObject = jsonObject;

      _name = jsonObject[@"name"];
      _selected = [jsonObject[@"selected"] boolValue];

      _simpleModifications = [self simpleModificationsDictionaryToArray:jsonObject[@"simple_modifications"]];
      _fnFunctionKeys = [self simpleModificationsDictionaryToArray:jsonObject[@"fn_function_keys"]];

      _virtualHIDKeyboardType = @"ansi";
      if ([jsonObject[@"virtual_hid_keyboard"] isKindOfClass:[NSDictionary class]]) {
        _virtualHIDKeyboardType = jsonObject[@"virtual_hid_keyboard"][@"keyboard_type"];
      }

      _virtualHIDKeyboardCapsLockDelayMilliseconds = 0;
      if ([jsonObject[@"virtual_hid_keyboard"] isKindOfClass:[NSDictionary class]]) {
        _virtualHIDKeyboardCapsLockDelayMilliseconds = [jsonObject[@"virtual_hid_keyboard"][@"caps_lock_delay_milliseconds"] unsignedIntegerValue];
      }

      // _devices
      NSMutableArray<KarabinerKitDeviceConfiguration*>* devices = [NSMutableArray new];
      if (jsonObject[@"devices"]) {
        for (NSDictionary* device in jsonObject[@"devices"]) {
          KarabinerKitDeviceConfiguration* deviceConfiguration = [[KarabinerKitDeviceConfiguration alloc] initWithJsonObject:device];

          if (deviceConfiguration.deviceIdentifiers) {
            BOOL found = NO;
            for (KarabinerKitDeviceConfiguration* d in devices) {
              if ([d.deviceIdentifiers isEqualToDeviceIdentifiers:deviceConfiguration.deviceIdentifiers]) {
                found = YES;
              }
            }

            if (!found) {
              [devices addObject:deviceConfiguration];
            }
          }
        }
      }
      _devices = devices;
    }
  }

  return self;
}

- (NSArray*)simpleModificationsDictionaryToArray:(NSDictionary*)dictionary {
  NSMutableArray<NSDictionary*>* array = [NSMutableArray new];

  if ([dictionary isKindOfClass:[NSDictionary class]]) {
    for (NSString* key in [[dictionary allKeys] sortedArrayUsingSelector:@selector(localizedStandardCompare:)]) {
      [array addObject:@{
        @"from" : key,
        @"to" : dictionary[key],
      }];
    }
  }

  return array;
}

- (void)addSimpleModification {
  NSMutableArray* simpleModifications = [NSMutableArray arrayWithArray:self.simpleModifications];
  [simpleModifications addObject:@{
    @"from" : @"",
    @"to" : @"",
  }];
  self.simpleModifications = simpleModifications;
}

- (void)removeSimpleModification:(NSUInteger)index {
  if (index < self.simpleModifications.count) {
    NSMutableArray* simpleModifications = [NSMutableArray arrayWithArray:self.simpleModifications];
    [simpleModifications removeObjectAtIndex:index];
    self.simpleModifications = simpleModifications;
  }
}

- (void)replaceSimpleModification:(NSUInteger)index from:(NSString*)from to:(NSString*)to {
  if (index < self.simpleModifications.count && from && to) {
    NSMutableArray* simpleModifications = [NSMutableArray arrayWithArray:self.simpleModifications];
    simpleModifications[index] = @{
      @"from" : from,
      @"to" : to,
    };
    self.simpleModifications = simpleModifications;
  }
}

- (void)replaceFnFunctionKey:(NSString*)from to:(NSString*)to {
  NSMutableArray* fnFunctionKeys = [NSMutableArray arrayWithArray:self.fnFunctionKeys];
  [fnFunctionKeys enumerateObjectsUsingBlock:^(id obj, NSUInteger index, BOOL* stop) {
    if ([obj[@"from"] isEqualToString:from]) {
      fnFunctionKeys[index] = @{
        @"from" : from,
        @"to" : to,
      };
      *stop = YES;
    }
  }];
  self.fnFunctionKeys = fnFunctionKeys;
}

- (void)setDeviceConfiguration:(KarabinerKitDeviceIdentifiers*)deviceIdentifiers
                            ignore:(BOOL)ignore
    disableBuiltInKeyboardIfExists:(BOOL)disableBuiltInKeyboardIfExists {
  NSMutableArray* devices = [NSMutableArray arrayWithArray:self.devices];
  BOOL __block found = NO;
  [devices enumerateObjectsUsingBlock:^(KarabinerKitDeviceConfiguration* obj, NSUInteger index, BOOL* stop) {
    if ([obj.deviceIdentifiers isEqualToDeviceIdentifiers:deviceIdentifiers]) {
      obj.ignore = ignore;
      obj.disableBuiltInKeyboardIfExists = disableBuiltInKeyboardIfExists;

      found = YES;
      *stop = YES;
    }
  }];

  if (!found) {
    KarabinerKitDeviceConfiguration* deviceConfiguration = [KarabinerKitDeviceConfiguration new];
    deviceConfiguration.deviceIdentifiers = deviceIdentifiers;
    deviceConfiguration.ignore = ignore;
    deviceConfiguration.disableBuiltInKeyboardIfExists = disableBuiltInKeyboardIfExists;
    [devices addObject:deviceConfiguration];
  }

  self.devices = devices;
}

- (NSDictionary*)simpleModificationsArrayToDictionary:(NSArray*)array {
  NSMutableDictionary* dictionary = [NSMutableDictionary new];
  for (NSDictionary* d in array) {
    NSString* from = d[@"from"];
    NSString* to = d[@"to"];
    if (from && ![from isEqualToString:@""] &&
        to && ![to isEqualToString:@""] &&
        !dictionary[from]) {
      dictionary[from] = to;
    }
  }
  return dictionary;
}

- (NSDictionary*)jsonObject {
  NSMutableDictionary* mutableJsonObject = [self.originalJsonObject mutableCopy];
  mutableJsonObject[@"name"] = self.name;

  mutableJsonObject[@"selected"] = @(self.selected);

  mutableJsonObject[@"simple_modifications"] = [self simpleModificationsArrayToDictionary:self.simpleModifications];

  mutableJsonObject[@"fn_function_keys"] = [self simpleModificationsArrayToDictionary:self.fnFunctionKeys];

  mutableJsonObject[@"virtual_hid_keyboard"] = @{
    @"keyboard_type" : self.virtualHIDKeyboardType,
    @"caps_lock_delay_milliseconds" : @(self.virtualHIDKeyboardCapsLockDelayMilliseconds),
  };

  NSMutableArray* devices = [NSMutableArray new];
  for (KarabinerKitDeviceConfiguration* d in self.devices) {
    [devices addObject:@{
      @"identifiers" : [d.deviceIdentifiers toDictionary],
      @"ignore" : @(d.ignore),
      @"disable_built_in_keyboard_if_exists" : @(d.disableBuiltInKeyboardIfExists),
    }];
  }
  mutableJsonObject[@"devices"] = devices;

  return mutableJsonObject;
}

@end

@interface KarabinerKitCoreConfigurationModel ()

@property KarabinerKitGlobalConfiguration* globalConfiguration;
@property(copy) NSArray<KarabinerKitConfigurationProfile*>* profiles;
@property KarabinerKitConfigurationProfile* currentProfile;

@end

@implementation KarabinerKitCoreConfigurationModel

- (instancetype)initWithJsonObject:(NSDictionary*)jsonObject {
  self = [super init];

  if (self) {
    _globalConfiguration = [[KarabinerKitGlobalConfiguration alloc] initWithJsonObject:jsonObject[@"global"]];

    NSMutableArray<KarabinerKitConfigurationProfile*>* mutableProfiles = [NSMutableArray<KarabinerKitConfigurationProfile*> new];

    NSArray* profiles = jsonObject[@"profiles"];
    if ([profiles isKindOfClass:[NSArray class]]) {
      for (NSDictionary* profile in profiles) {
        KarabinerKitConfigurationProfile* p = [[KarabinerKitConfigurationProfile alloc] initWithJsonObject:profile];
        [mutableProfiles addObject:p];

        if (p.selected) {
          _currentProfile = p;
        }
      }
    }

    _profiles = mutableProfiles;
  }

  return self;
}

- (void)addProfile {
  NSDictionary* jsonObject = [KarabinerKitJsonUtility loadCString:libkrbn_get_default_profile_json_string()];
  KarabinerKitConfigurationProfile* profile = [[KarabinerKitConfigurationProfile alloc] initWithJsonObject:jsonObject];

  profile.name = @"New profile";
  profile.selected = NO;

  NSMutableArray<KarabinerKitConfigurationProfile*>* mutableProfiles = [self.profiles mutableCopy];
  [mutableProfiles addObject:profile];

  self.profiles = mutableProfiles;
}

- (void)removeProfile:(NSUInteger)index {
  if (index < self.profiles.count) {
    NSMutableArray<KarabinerKitConfigurationProfile*>* mutableProfiles = [self.profiles mutableCopy];
    [mutableProfiles removeObjectAtIndex:index];
    self.profiles = mutableProfiles;
  }
}

- (void)selectProfile:(NSUInteger)index {
  if (index < self.profiles.count) {
    [self.profiles enumerateObjectsUsingBlock:^(KarabinerKitConfigurationProfile* profile, NSUInteger i, BOOL* stop) {
      if (index == i) {
        profile.selected = YES;
        self.currentProfile = profile;
      } else {
        profile.selected = NO;
      }
    }];
  }
}

@end

@interface KarabinerKitCoreConfigurationModel2 ()

@property libkrbn_core_configuration* libkrbn_core_configuration;

@end

@implementation KarabinerKitCoreConfigurationModel2

- (instancetype)init {
  self = [super init];

  if (self) {
    libkrbn_core_configuration* p = NULL;
    if (libkrbn_core_configuration_initialize(&p, libkrbn_get_core_configuration_file_path())) {
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

- (NSUInteger) selectedProfileFnFunctionKeysCount {
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
