// -*- mode: objective-c -*-

@import Cocoa;

@interface EventQueue : NSObject

@property(readonly) BOOL observed;

- (void)setup;
- (void)pushMouseEvent:(NSEvent*)event;

@end
