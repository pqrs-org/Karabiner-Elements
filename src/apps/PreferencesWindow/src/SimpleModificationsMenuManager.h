// -*- Mode: objc -*-

@import Cocoa;

@interface SimpleModificationsMenuManager : NSObject

@property(readonly) NSMenu* fromMenu;
@property(readonly) NSMenu* toMenu;
@property(readonly) NSMenu* toMenuWithInherited;

- (void)setup;

@end
