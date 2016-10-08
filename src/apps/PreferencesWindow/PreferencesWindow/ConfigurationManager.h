// -*- Mode: objc -*-

@import Cocoa;

@interface ConfigurationManager : NSObject

@property(copy, readonly) NSDictionary* currentProfile;

- (void)setup;

@end
