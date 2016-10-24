// -*- Mode: objc -*-

@import Cocoa;
#import "ConfigurationCoreModel.h"

@interface ConfigurationManager : NSObject

@property(readonly) ConfigurationCoreModel* configurationCoreModel;

- (void)setup;

- (void)save;

@end
