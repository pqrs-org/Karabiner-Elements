// -*- Mode: objc -*-

@import Cocoa;
#import "CoreConfigurationModel.h"

@interface KarabinerKitConfigurationManager : NSObject

@property(readonly) KarabinerKitCoreConfigurationModel* coreConfigurationModel;
@property(readonly) KarabinerKitCoreConfigurationModel2* coreConfigurationModel2;

+ (KarabinerKitConfigurationManager*)sharedManager;

- (void)save;

@end
