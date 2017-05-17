// -*- Mode: objc -*-

@import Cocoa;

@interface SimpleModificationsMenuManager : NSObject

@property(readonly) NSMenu* fromMenu;
@property(readonly) NSMenu* toMenu;
@property(readonly) NSMenu* vendorIdMenu;
@property(readonly) NSMenu* productIdMenu;

- (void)setup;
- (void)setupVendor;

@end
