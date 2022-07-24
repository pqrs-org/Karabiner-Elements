// -*- mode: objective-c -*-

@import AppKit;

@interface OpenAtLoginObjc : NSObject

+ (BOOL)enabled:(NSURL *)appURL;

+ (void)enable:(NSURL *)appURL;
+ (void)disable:(NSURL *)appURL;

@end
