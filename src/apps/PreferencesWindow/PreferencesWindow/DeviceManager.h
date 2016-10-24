// -*- Mode: objc -*-

@import Cocoa;
#import "DeviceModel.h"

@interface DeviceManager : NSObject

@property(copy, readonly) NSArray<DeviceModel*>* deviceModels;

- (void)setup;

@end
