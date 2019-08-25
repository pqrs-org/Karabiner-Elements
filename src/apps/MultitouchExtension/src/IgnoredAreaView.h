// -*- mode: objective-c -*-

@import Cocoa;

@interface IgnoredAreaView : NSView

- (void)clearFingers;
- (void)addFinger:(NSPoint)point ignored:(BOOL)ignored;
+ (BOOL)isIgnoredArea:(NSPoint)point;

@end
