// -*- mode: objective-c -*-

@import Cocoa;

@interface PreferencesController : NSObject

- (void)load;
- (void)show;
+ (BOOL)isSettingEnabled:(NSInteger)fingers;
+ (NSString*)getSettingIdentifier:(NSInteger)fingers;

@end
