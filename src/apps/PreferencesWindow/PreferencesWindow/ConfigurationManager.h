// -*- Mode: objc -*-

@import Cocoa;
#import "CoreConfigurationModel.h"

@interface ConfigurationManager : NSObject

@property(readonly) CoreConfigurationModel* configurationCoreModel;

- (void)setup;

- (void)save;

@end
