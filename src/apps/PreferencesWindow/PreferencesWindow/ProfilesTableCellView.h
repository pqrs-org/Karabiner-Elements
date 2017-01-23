// -*- Mode: objc -*-

@import Cocoa;

@interface ProfilesTableCellView : NSTableCellView

@property(weak) IBOutlet NSTextField* name;
@property(weak) IBOutlet NSTextField* selected;
@property(weak) IBOutlet NSButton* removeProfileButton;

@end
