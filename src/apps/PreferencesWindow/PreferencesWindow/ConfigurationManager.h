// -*- Mode: objc -*-

@import Cocoa;
#import "KarabinerKit/KarabinerKit.h"

@interface ConfigurationManager : NSObject

@property(readonly) KarabinerKitCoreConfigurationModel* coreConfigurationModel;

- (void)setup;

- (void)save;

@end
