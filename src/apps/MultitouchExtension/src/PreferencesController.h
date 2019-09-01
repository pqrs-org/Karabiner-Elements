// -*- mode: objective-c -*-

@import Cocoa;

@interface PreferencesController : NSObject

- (void)show;
+ (BOOL)isSettingEnabled:(NSInteger)fingers;
+ (NSString*)getSettingIdentifier:(NSInteger)fingers;
+ (NSRect)makeTargetArea;

@end
