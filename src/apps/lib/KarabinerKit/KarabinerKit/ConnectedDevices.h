// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn.h"

@interface KarabinerKitConnectedDevices : NSObject

- (instancetype)initWithInitializedConnectedDevices:(libkrbn_connected_devices*)initializedConnectedDevices;

@property(readonly) NSUInteger devicesCount;
- (NSString*)manufacturerAtIndex:(NSUInteger)index;
- (NSString*)productAtIndex:(NSUInteger)index;
- (NSUInteger)vendorIdAtIndex:(NSUInteger)index;
- (NSUInteger)productIdAtIndex:(NSUInteger)index;
- (BOOL)isKeyboardAtIndex:(NSUInteger)index;
- (BOOL)isPointingDeviceAtIndex:(NSUInteger)index;
- (BOOL)isBuiltInKeyboardAtIndex:(NSUInteger)index;

@end
