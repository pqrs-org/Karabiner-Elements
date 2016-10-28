#import "DeviceModel.h"

@interface DeviceIdentifiers ()

@property(readwrite) uint32_t vendorId;
@property(readwrite) uint32_t productId;
@property(readwrite) BOOL isKeyboard;
@property(readwrite) BOOL isPointingDevice;

@end

@implementation DeviceIdentifiers

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

- (BOOL)isEqualToDeviceIdentifiers:(DeviceIdentifiers*)other {
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

@interface DeviceDescriptions ()

@property(copy, readwrite) NSString* manufacturer;
@property(copy, readwrite) NSString* product;

@end

@implementation DeviceDescriptions

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

@interface DeviceModel ()

@property(readwrite) DeviceIdentifiers* deviceIdentifiers;
@property(readwrite) DeviceDescriptions* deviceDescriptions;
@property(readwrite) BOOL ignore;
@property(readwrite) uint32_t keyboardType;

@end

@implementation DeviceModel

- (instancetype)initWithDictionary:(NSDictionary*)device {
  self = [super init];

  if (self) {
    _deviceIdentifiers = [[DeviceIdentifiers alloc] initWithDictionary:device[@"identifiers"]];
    _deviceDescriptions = [[DeviceDescriptions alloc] initWithDictionary:device[@"descriptions"]];
    _ignore = [device[@"ignore"] boolValue];
    _keyboardType = [device[@"keyboard_type"] unsignedIntValue];
  }

  return self;
}

@end
