#import "ConfigurationCoreModel.h"

@interface ConfigurationCoreModel ()

@property(copy, readwrite) NSArray<NSDictionary*>* simpleModifications;

@end

@implementation ConfigurationCoreModel

- (instancetype)initWithProfile:(NSDictionary*)profile {
  self = [super init];

  if (self) {
    NSMutableArray<NSDictionary*>* simpleModifications = [NSMutableArray new];

    for (NSString* key in [[profile[@"simple_modifications"] allKeys] sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)]) {
      [simpleModifications addObject:@{
        @"from" : key,
        @"to" : profile[@"simple_modifications"][key],
      }];
    }

    self.simpleModifications = simpleModifications;
  }

  return self;
}

#pragma mark - NSCoping

- (id)copyWithZone:(NSZone*)zone {
  ConfigurationCoreModel* obj = [[[self class] allocWithZone:zone] init];
  if (obj) {
    obj.simpleModifications = [self.simpleModifications copyWithZone:zone];
  }
  return obj;
}

@end
