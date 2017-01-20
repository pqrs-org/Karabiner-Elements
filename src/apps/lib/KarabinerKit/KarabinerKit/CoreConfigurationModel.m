#import "CoreConfigurationModel.h"
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
    _showInMenubar = YES;

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
          _showInMenubar = [value boolValue];
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

@property(copy, readwrite) NSArray<NSDictionary*>* simpleModifications;
@property(copy, readwrite) NSArray<NSDictionary*>* fnFunctionKeys;
@property(copy, readwrite) NSArray<KarabinerKitDeviceConfiguration*>* devices;

@end

@implementation KarabinerKitConfigurationProfile

- (instancetype)initWithJsonObject:(NSDictionary*)jsonObject {
  self = [super init];

  if (self) {
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

- (NSDictionary*)simpleModificationsDictionary {
  return [self simpleModificationsArrayToDictionary:self.simpleModifications];
}

- (NSDictionary*)fnFunctionKeysDictionary {
  return [self simpleModificationsArrayToDictionary:self.fnFunctionKeys];
}

- (NSDictionary*)virtualHIDKeyboardDictionary {
  return @{
    @"keyboard_type" : self.virtualHIDKeyboardType,
    @"caps_lock_delay_milliseconds" : @(self.virtualHIDKeyboardCapsLockDelayMilliseconds),
  };
}

- (NSArray*)devicesArray {
  NSMutableArray* array = [NSMutableArray new];
  for (KarabinerKitDeviceConfiguration* d in self.devices) {
    [array addObject:@{
      @"identifiers" : [d.deviceIdentifiers toDictionary],
      @"ignore" : @(d.ignore),
      @"disable_built_in_keyboard_if_exists" : @(d.disableBuiltInKeyboardIfExists),
    }];
  }
  return array;
}

@end

@interface KarabinerKitCoreConfigurationModel ()

@property KarabinerKitGlobalConfiguration* globalConfiguration;
@property KarabinerKitConfigurationProfile* currentProfile;

@end

@implementation KarabinerKitCoreConfigurationModel

- (instancetype)initWithJsonObject:(NSDictionary*)jsonObject {
  self = [super init];

  if (self) {
    _globalConfiguration = [[KarabinerKitGlobalConfiguration alloc] initWithJsonObject:jsonObject[@"global"]];

    NSArray* profiles = jsonObject[@"profiles"];
    if ([profiles isKindOfClass:[NSArray class]]) {
      for (NSDictionary* profile in profiles) {
        KarabinerKitConfigurationProfile* p = [[KarabinerKitConfigurationProfile alloc] initWithJsonObject:profile];
        if (p.selected) {
          _currentProfile = p;
        }
      }
    }
  }

  return self;
}

@end
