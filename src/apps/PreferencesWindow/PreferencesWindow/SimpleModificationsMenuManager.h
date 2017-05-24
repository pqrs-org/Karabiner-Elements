// -*- Mode: objc -*-

@import Cocoa;

@interface SimpleModificationsMenuManager : NSObject

@property(readonly) NSMenu* fromMenu;
@property(readonly) NSMenu* toMenu;
@property(readonly) NSMenu* vendorIdMenu;

- (void)setup;
- (void)setupVendor;

@end
