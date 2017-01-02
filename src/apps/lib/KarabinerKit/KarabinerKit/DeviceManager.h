// -*- Mode: objc -*-

@import Foundation;
#import "DeviceModel.h"

@interface KarabinerKitDeviceManager : NSObject

@property(copy, readonly) NSArray<KarabinerKitDeviceModel*>* deviceModels;

+ (KarabinerKitDeviceManager*)sharedManager;

@end
