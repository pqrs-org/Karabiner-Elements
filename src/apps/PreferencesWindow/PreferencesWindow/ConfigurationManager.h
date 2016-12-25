// -*- Mode: objc -*-

@import Cocoa;
#import "CoreConfigurationModel.h"

@interface ConfigurationManager : NSObject

@property(readonly) CoreConfigurationModel* coreConfigurationModel;

- (void)setup;

- (void)save;

@end
