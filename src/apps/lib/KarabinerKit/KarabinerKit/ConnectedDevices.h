// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn.h"

@interface KarabinerKitConnectedDevices : NSObject

- (instancetype)initWithInitializedConnectedDevices:(libkrbn_connected_devices*)initializedConnectedDevices;

@property(readonly) NSUInteger devicesCount;
- (NSString*)manufacturerAtIndex:(NSUInteger)index;
- (NSString*)productAtIndex:(NSUInteger)index;
- (libkrbn_device_identifiers)deviceIdentifiersAtIndex:(NSUInteger)index;
- (BOOL)isBuiltInKeyboardAtIndex:(NSUInteger)index;

@end
