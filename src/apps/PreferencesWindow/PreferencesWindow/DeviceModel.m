#import "DeviceModel.h"

@interface DeviceModel ()

@property(copy, readwrite) NSString* manufacturer;
@property(copy, readwrite) NSString* product;
@property(readwrite) BOOL ignore;
@property(readwrite) uint32_t productId;
@property(readwrite) uint32_t vendorId;

@end

@implementation DeviceModel

- (instancetype)initWithDevice:(NSDictionary*)device {
  self = [super init];

  if (self) {
    _manufacturer = device[@"manufacturer"];
    _product = device[@"product"];
    _ignore = [device[@"ignore"] boolValue];
    _productId = [device[@"product_id"] unsignedIntValue];
    _vendorId = [device[@"vendor_id"] unsignedIntValue];
  }

  return self;
}

@end
