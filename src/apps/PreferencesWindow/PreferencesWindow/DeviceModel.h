// -*- Mode: objc -*-

@import Cocoa;

@interface DeviceIdentifiers : NSObject

@property(readonly) uint32_t vendorId;
@property(readonly) uint32_t productId;
@property(readonly) BOOL isKeyboard;
@property(readonly) BOOL isPointingDevice;

- (instancetype)initWithDictionary:(NSDictionary*)dictionary;
- (BOOL)isEqualToDeviceIdentifiers:(DeviceIdentifiers*)other;
- (NSDictionary*)toDictionary;

@end

@interface DeviceDescriptions : NSObject

@property(copy, readonly) NSString* manufacturer;
@property(copy, readonly) NSString* product;

- (instancetype)initWithDictionary:(NSDictionary*)dictionary;

@end

@interface DeviceModel : NSObject

@property(readonly) DeviceIdentifiers* deviceIdentifiers;
@property(readonly) DeviceDescriptions* deviceDescriptions;
@property(readonly) BOOL ignore;

- (instancetype)initWithDictionary:(NSDictionary*)dictionary;

@end
