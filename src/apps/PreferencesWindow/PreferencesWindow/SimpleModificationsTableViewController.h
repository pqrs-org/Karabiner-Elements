// -*- Mode: objc -*-

@import Cocoa;

@interface SimpleModificationsTableViewController : NSObject

+ (void)selectPopUpButtonMenu:(NSPopUpButton*)popUpButton representedObject:(NSObject*)representedObject;

- (void)setup;
- (void)valueChanged:(id)sender;
- (void)removeItem:(id)sender;
- (void)vendorProductIdChanged:(id)sender;

@end
