// -*- mode: objective-c -*-

@import Cocoa;
#import "CoreConfigurationModel.h"

@interface KarabinerKitConfigurationManager : NSObject

@property(readonly) KarabinerKitCoreConfigurationModel* coreConfigurationModel;

+ (KarabinerKitConfigurationManager*)sharedManager;

@end
