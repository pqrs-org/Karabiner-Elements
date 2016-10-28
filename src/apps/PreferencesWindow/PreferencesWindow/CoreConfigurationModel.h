// -*- Mode: objc -*-

@import Cocoa;
#import "DeviceModel.h"

@interface DeviceConfiguration : NSObject

@property(readonly) DeviceIdentifiers* deviceIdentifiers;
@property BOOL ignore;
@property uint32_t keyboardType;

@end

@interface CoreConfigurationModel : NSObject

@property(copy, readonly) NSArray<NSDictionary*>* simpleModifications;
@property(copy, readonly) NSArray<NSDictionary*>* fnFunctionKeys;
@property(copy, readonly) NSArray<DeviceConfiguration*>* devices;
@property(copy, readonly) NSDictionary* simpleModificationsDictionary;
@property(copy, readonly) NSDictionary* fnFunctionKeysDictionary;
@property(copy, readonly) NSArray* devicesArray;

- (instancetype)initWithProfile:(NSDictionary*)profile;

- (void)addSimpleModification;
- (void)removeSimpleModification:(NSUInteger)index;
- (void)replaceSimpleModification:(NSUInteger)index from:(NSString*)from to:(NSString*)to;

- (void)replaceFnFunctionKey:(NSString*)from to:(NSString*)to;

- (void)setDeviceConfiguration:(DeviceIdentifiers*)deviceIdentifiers ignore:(BOOL)ignore keyboardType:(uint32_t)keyboardType;

@end
