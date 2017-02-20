// -*- Mode: objc -*-

@import Cocoa;
#import "CoreConfigurationModel.h"

@interface KarabinerKitConfigurationManager : NSObject

@property(readonly) KarabinerKitCoreConfigurationModel2* coreConfigurationModel2;

+ (KarabinerKitConfigurationManager*)sharedManager;

@end
