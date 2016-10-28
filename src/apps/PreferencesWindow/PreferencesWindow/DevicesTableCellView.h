// -*- Mode: objc -*-

@import Cocoa;
#import "DeviceModel.h"

@interface DevicesTableCellView : NSTableCellView

// for DevicesCheckboxColumn
@property(weak) IBOutlet NSButton* checkbox;
// for DevicesKeyboardTypeColumn
@property(weak) IBOutlet NSPopUpButton* popUpButton;
// for DevicesIconsColumn
@property(weak) IBOutlet NSImageView* keyboardImage;
@property(weak) IBOutlet NSImageView* mouseImage;

@property DeviceIdentifiers* deviceIdentifiers;

@end
