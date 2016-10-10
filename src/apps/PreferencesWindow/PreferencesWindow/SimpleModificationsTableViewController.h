// -*- Mode: objc -*-

@import Cocoa;

@interface SimpleModificationsTableViewController : NSObject

+ (void)selectPopUpButtonMenu:(NSPopUpButton*)popUpButton representedObject:(NSString*)representedObject;

- (void)setup;
- (void)valueChanged:(id)sender;
- (void)removeItem:(id)sender;

@end
