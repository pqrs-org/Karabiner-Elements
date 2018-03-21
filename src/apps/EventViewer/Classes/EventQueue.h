/* -*- Mode: objc; Coding: utf-8; indent-tabs-mode: nil; -*- */

@import Cocoa;

@interface EventQueue : NSObject

- (void)pushMouseEvent:(NSEvent*)event;

@end
