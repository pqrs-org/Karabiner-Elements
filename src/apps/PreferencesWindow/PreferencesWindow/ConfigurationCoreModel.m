#import "ConfigurationCoreModel.h"

@interface ConfigurationCoreModel ()

@property(copy, readwrite) NSArray<NSDictionary*>* simpleModifications;
@property(copy, readwrite) NSArray<NSDictionary*>* fnFunctionKeys;

@end

@implementation ConfigurationCoreModel

- (instancetype)initWithProfile:(NSDictionary*)profile {
  self = [super init];

  if (self) {
    _simpleModifications = [self simpleModificationsDictionaryToArray:profile[@"simple_modifications"]];
    _fnFunctionKeys = [self simpleModificationsDictionaryToArray:profile[@"fn_function_keys"]];
  }

  return self;
}

- (NSArray*)simpleModificationsDictionaryToArray:(NSDictionary*)dictionary {
  NSMutableArray<NSDictionary*>* array = [NSMutableArray new];

  for (NSString* key in [[dictionary allKeys] sortedArrayUsingSelector:@selector(localizedStandardCompare:)]) {
    [array addObject:@{
      @"from" : key,
      @"to" : dictionary[key],
    }];
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

@end
