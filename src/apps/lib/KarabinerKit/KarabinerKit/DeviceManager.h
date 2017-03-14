// -*- Mode: objc -*-

@import Foundation;
#import "ConnectedDevices.h"

@interface KarabinerKitDeviceManager : NSObject

@property(readonly) KarabinerKitConnectedDevices* connectedDevices;

+ (KarabinerKitDeviceManager*)sharedManager;

@end
