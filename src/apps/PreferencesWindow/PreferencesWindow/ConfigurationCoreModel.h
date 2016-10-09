// -*- Mode: objc -*-

@import Cocoa;

@interface ConfigurationCoreModel : NSObject <NSCopying>

@property(copy, readonly) NSArray<NSDictionary*>* simpleModifications;

- (instancetype)initWithProfile:(NSDictionary*)profile;

@end
