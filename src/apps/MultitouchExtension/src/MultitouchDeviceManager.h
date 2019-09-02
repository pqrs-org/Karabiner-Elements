// -*- mode: objective-c -*-

@import Cocoa;

@interface MultitouchDeviceManager : NSObject

+ (instancetype)sharedMultitouchDeviceManager;

- (void)setCallback:(BOOL)set;

- (void)registerIONotification;
- (void)unregisterIONotification;

- (void)registerWakeNotification;
- (void)unregisterWakeNotification;

@end
