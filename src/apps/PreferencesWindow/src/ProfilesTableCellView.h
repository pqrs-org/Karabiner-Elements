// -*- mode: objective-c -*-

@import Cocoa;

@interface ProfilesTableCellView : NSTableCellView

@property(weak) IBOutlet NSTextField* name;
@property(weak) IBOutlet NSImageView* selectedImage;
@property(weak) IBOutlet NSTextField* selected;
@property(weak) IBOutlet NSButton* selectProfileButton;
@property(weak) IBOutlet NSButton* removeProfileButton;

@end
