// -*- Mode: objc -*-

@import Cocoa;
#import "ConfigurationCoreModel.h"

@interface ConfigurationManager : NSObject

@property(copy, readonly) ConfigurationCoreModel* configurationCoreModel;

- (void)setup;

@end
