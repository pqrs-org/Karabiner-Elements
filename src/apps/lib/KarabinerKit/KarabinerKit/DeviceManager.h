// -*- Mode: objc -*-

@import Foundation;
#import "ConnectedDevices.h"
#import "DeviceModel.h"

@interface KarabinerKitDeviceManager : NSObject

@property(copy, readonly) NSArray<KarabinerKitDeviceModel*>* deviceModels;
@property(readonly) KarabinerKitConnectedDevices* connectedDevices;

+ (KarabinerKitDeviceManager*)sharedManager;

@end
