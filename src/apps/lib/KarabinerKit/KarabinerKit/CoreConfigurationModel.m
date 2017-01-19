#import "CoreConfigurationModel.h"
#import "libkrbn.h"

@implementation KarabinerKitGlobalConfiguration
@end

@interface KarabinerKitDeviceConfiguration ()

@property(readwrite) KarabinerKitDeviceIdentifiers* deviceIdentifiers;

@end

@implementation KarabinerKitDeviceConfiguration
@end

@interface KarabinerKitCoreConfigurationModel ()

@property(copy, readwrite) NSArray<NSDictionary*>* simpleModifications;
@property(copy, readwrite) NSArray<NSDictionary*>* fnFunctionKeys;
@property(copy, readwrite) NSArray<KarabinerKitDeviceConfiguration*>* devices;

@end

@implementation KarabinerKitCoreConfigurationModel

- (instancetype)initWithProfile:(NSDictionary*)jsonObject currentProfileJsonObject:(NSDictionary*)profile {
  self = [super init];

  if (self) {
    _globalConfiguration = [KarabinerKitGlobalConfiguration new];
    _globalConfiguration.checkForUpdatesOnStartup = YES;
    _globalConfiguration.showInMenubar = YES;

    {
      NSDictionary* global = jsonObject[@"global"];
      if ([global isKindOfClass:[NSDictionary class]]) {
        {
          NSNumber* value = global[@"check_for_updates_on_startup"];
          if (value) {
            _globalConfiguration.checkForUpdatesOnStartup = [value boolValue];
          }
        }
        {
          NSNumber* value = global[@"show_in_menu_bar"];
          if (value) {
            _globalConfiguration.showInMenubar = [value boolValue];
          }
        }
      }
    }

    _simpleModifications = [self simpleModificationsDictionaryToArray:profile[@"simple_modifications"]];
    _fnFunctionKeys = [self simpleModificationsDictionaryToArray:profile[@"fn_function_keys"]];

    _virtualHIDKeyboardType = @"ansi";
    if ([profile[@"virtual_hid_keyboard"] isKindOfClass:[NSDictionary class]]) {
      _virtualHIDKeyboardType = profile[@"virtual_hid_keyboard"][@"keyboard_type"];
    }

    _virtualHIDKeyboardCapsLockDelayMilliseconds = 0;
    if ([profile[@"virtual_hid_keyboard"] isKindOfClass:[NSDictionary class]]) {
      _virtualHIDKeyboardCapsLockDelayMilliseconds = [profile[@"virtual_hid_keyboard"][@"caps_lock_delay_milliseconds"] unsignedIntegerValue];
    }

    // _devices
    NSMutableArray<KarabinerKitDeviceConfiguration*>* devices = [NSMutableArray new];
    if (profile[@"devices"]) {
      for (NSDictionary* device in profile[@"devices"]) {
        NSDictionary* identifiers = device[@"identifiers"];
        if (!identifiers) {
          continue;
        }
        KarabinerKitDeviceIdentifiers* deviceIdentifiers = [[KarabinerKitDeviceIdentifiers alloc] initWithDictionary:identifiers];

        BOOL found = NO;
        for (KarabinerKitDeviceConfiguration* d in devices) {
          if ([d.deviceIdentifiers isEqualToDeviceIdentifiers:deviceIdentifiers]) {
            found = YES;
          }
        }

        if (!found) {
          KarabinerKitDeviceConfiguration* deviceConfiguration = [KarabinerKitDeviceConfiguration new];
          deviceConfiguration.deviceIdentifiers = deviceIdentifiers;
          deviceConfiguration.ignore = [device[@"ignore"] boolValue];
          deviceConfiguration.disableBuiltInKeyboardIfExists = [device[@"disable_built_in_keyboard_if_exists"] boolValue];
          [devices addObject:deviceConfiguration];
        }
      }
    }
    _devices = devices;
  }

  return self;
}

- (NSArray*)simpleModificationsDictionaryToArray:(NSDictionary*)dictionary {
  NSMutableArray<NSDictionary*>* array = [NSMutableArray new];

  if (dictionary) {
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
