#import "ConfigurationCoreModel.h"

@interface DeviceConfiguration ()

@property(readwrite) DeviceIdentifiers* deviceIdentifiers;

@end

@implementation DeviceConfiguration
@end

@interface ConfigurationCoreModel ()

@property(copy, readwrite) NSArray<NSDictionary*>* simpleModifications;
@property(copy, readwrite) NSArray<NSDictionary*>* fnFunctionKeys;
@property(copy, readwrite) NSArray<DeviceConfiguration*>* devices;

@end

@implementation ConfigurationCoreModel

- (instancetype)initWithProfile:(NSDictionary*)profile {
  self = [super init];

  if (self) {
    _simpleModifications = [self simpleModificationsDictionaryToArray:profile[@"simple_modifications"]];
    _fnFunctionKeys = [self simpleModificationsDictionaryToArray:profile[@"fn_function_keys"]];

    // _devices
    NSMutableArray<DeviceConfiguration*>* devices = [NSMutableArray new];
    if (profile[@"devices"]) {
      for (NSDictionary* device in profile[@"devices"]) {
        DeviceIdentifiers* deviceIdentifiers = [[DeviceIdentifiers alloc] initWithDictionary:device];

        BOOL found = NO;
        for (DeviceConfiguration* d in devices) {
          if ([d.deviceIdentifiers isEqualToDeviceIdentifiers:deviceIdentifiers]) {
            found = YES;
          }
        }

        if (!found) {
          DeviceConfiguration* deviceConfiguration = [DeviceConfiguration new];
          deviceConfiguration.deviceIdentifiers = deviceIdentifiers;
          deviceConfiguration.ignore = [device[@"ignore"] boolValue];
          [devices addObject:deviceConfiguration];
        }
      }
    }
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

- (void)setDeviceIgnore:(BOOL)ignore deviceIdentifiers:(DeviceIdentifiers*)deviceIdentifiers {
  NSMutableArray* devices = [NSMutableArray arrayWithArray:self.devices];
  BOOL __block found = NO;
  [devices enumerateObjectsUsingBlock:^(DeviceConfiguration* obj, NSUInteger index, BOOL* stop) {
    if ([obj.deviceIdentifiers isEqualToDeviceIdentifiers:deviceIdentifiers]) {
      obj.ignore = ignore;

      found = YES;
      *stop = YES;
    }
  }];

  if (!found) {
    DeviceConfiguration* deviceConfiguration = [DeviceConfiguration new];
    deviceConfiguration.deviceIdentifiers = deviceIdentifiers;
    deviceConfiguration.ignore = ignore;
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

- (NSArray*)devicesArray {
  NSMutableArray* array = [NSMutableArray new];
  for (DeviceConfiguration* d in self.devices) {
    [array addObject:@{
      @"identifiers" : [d.deviceIdentifiers toDictionary],
      @"ignore" : @(d.ignore),
    }];
  }
  return array;
}

@end
