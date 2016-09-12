// -*- Mode: objc; Coding: utf-8; indent-tabs-mode: nil; -*-

@import Cocoa;

@interface Relauncher : NSObject

+ (NSInteger)getRelaunchedCount;
+ (void)resetRelaunchedCount;
+ (void)relaunch;
+ (BOOL)isEqualPreviousProcessVersionAndCurrentProcessVersion;

@end
