// -*- Mode: objc -*-

@import Cocoa;

@interface KarabinerKitDeviceIdentifiers : NSObject

@property(readonly) uint32_t vendorId;
@property(readonly) uint32_t productId;
@property(readonly) BOOL isKeyboard;
@property(readonly) BOOL isPointingDevice;

- (instancetype)initWithDictionary:(NSDictionary*)dictionary;
- (BOOL)isEqualToDeviceIdentifiers:(KarabinerKitDeviceIdentifiers*)other;
- (NSDictionary*)toDictionary;

@end

@interface KarabinerKitDeviceDescriptions : NSObject

@property(copy, readonly) NSString* manufacturer;
@property(copy, readonly) NSString* product;

- (instancetype)initWithDictionary:(NSDictionary*)dictionary;

@end

@interface KarabinerKitDeviceModel : NSObject

@property(readonly) KarabinerKitDeviceIdentifiers* deviceIdentifiers;
@property(readonly) KarabinerKitDeviceDescriptions* deviceDescriptions;
@property(readonly) BOOL ignore;
@property(readonly) BOOL isBuiltInKeyboard;

- (instancetype)initWithDictionary:(NSDictionary*)dictionary;

@end
