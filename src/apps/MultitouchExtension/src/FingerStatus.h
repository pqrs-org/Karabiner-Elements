// -*- mode: objective-c -*-

@import Cocoa;

@interface FingerStatus : NSObject

- (void)clear;
- (void)add:(int)identifier active:(BOOL)active;
- (BOOL)isActive:(int)identifier;

@end
