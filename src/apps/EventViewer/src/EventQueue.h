/* -*- Mode: objc; Coding: utf-8; indent-tabs-mode: nil; -*- */

@import Cocoa;

@interface EventQueue : NSObject

@property(readonly) NSInteger observedDeviceCount;

- (void)setup;
- (void)pushMouseEvent:(NSEvent*)event;

@end
