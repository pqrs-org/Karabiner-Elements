// -*- Mode: objc -*-

@import Cocoa;

@interface DeviceModel : NSObject

@property bool ignore;
@property(copy, readonly) NSString* manufacturer;
@property(copy, readonly) NSString* product;
@property(readonly) uint32_t productId;
@property(readonly) uint32_t vendorId;

- (instancetype)initWithDevice:(NSDictionary*)device;

@end
