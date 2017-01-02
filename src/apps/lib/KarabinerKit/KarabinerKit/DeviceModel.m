#import "DeviceModel.h"

@interface KarabinerKitDeviceIdentifiers ()

@property(readwrite) uint32_t vendorId;
@property(readwrite) uint32_t productId;
@property(readwrite) BOOL isKeyboard;
@property(readwrite) BOOL isPointingDevice;

@end

@implementation KarabinerKitDeviceIdentifiers

- (instancetype)initWithDictionary:(NSDictionary*)dictionary {
  self = [super init];

  if (self) {
    if (dictionary) {
      _vendorId = [dictionary[@"vendor_id"] unsignedIntValue];
      _productId = [dictionary[@"product_id"] unsignedIntValue];
      _isKeyboard = [dictionary[@"is_keyboard"] boolValue];
      _isPointingDevice = [dictionary[@"is_pointing_device"] boolValue];
    }
  }

  return self;
}

- (BOOL)isEqualToDeviceIdentifiers:(KarabinerKitDeviceIdentifiers*)other {
  return self.vendorId == other.vendorId &&
         self.productId == other.productId &&
         self.isKeyboard == other.isKeyboard &&
         self.isPointingDevice == other.isPointingDevice;
}

- (NSDictionary*)toDictionary {
  return @{
    @"vendor_id" : @(self.vendorId),
    @"product_id" : @(self.productId),
    @"is_keyboard" : @(self.isKeyboard),
    @"is_pointing_device" : @(self.isPointingDevice),
  };
}

@end

@interface KarabinerKitDeviceDescriptions ()

@property(copy, readwrite) NSString* manufacturer;
@property(copy, readwrite) NSString* product;

@end

@implementation KarabinerKitDeviceDescriptions

- (instancetype)initWithDictionary:(NSDictionary*)dictionary {
  self = [super init];

  if (self) {
    if (dictionary) {
      _manufacturer = dictionary[@"manufacturer"];
      _product = dictionary[@"product"];
    }
  }

  return self;
}

@end

@interface KarabinerKitDeviceModel ()

@property(readwrite) KarabinerKitDeviceIdentifiers* deviceIdentifiers;
@property(readwrite) KarabinerKitDeviceDescriptions* deviceDescriptions;
@property(readwrite) BOOL ignore;
@property(readwrite) BOOL isBuiltInKeyboard;

@end

@implementation KarabinerKitDeviceModel

- (instancetype)initWithDictionary:(NSDictionary*)device {
  self = [super init];

  if (self) {
    _deviceIdentifiers = [[KarabinerKitDeviceIdentifiers alloc] initWithDictionary:device[@"identifiers"]];
    _deviceDescriptions = [[KarabinerKitDeviceDescriptions alloc] initWithDictionary:device[@"descriptions"]];
    _ignore = [device[@"ignore"] boolValue];
    _isBuiltInKeyboard = [device[@"is_built_in_keyboard"] boolValue];
  }

  return self;
}

@end
