// -*- mode: objective-c -*-

@import Cocoa;

enum {
  MAX_FINGER_COUNT = 4,
};

@interface PreferencesController : NSObject

- (void)show;
+ (BOOL)isSettingEnabled:(NSInteger)fingers;
+ (NSString*)getSettingIdentifier:(NSInteger)fingers;
+ (NSRect)makeTargetArea;

@end
