// -*- Mode: objc -*-

@import Cocoa;
#import "KarabinerKit/DeviceModel.h"

@interface DevicesTableCellView : NSTableCellView

// for DevicesCheckboxColumn
@property(weak) IBOutlet NSButton* checkbox;
// for DevicesKeyboardTypeColumn
@property(weak) IBOutlet NSPopUpButton* popUpButton;
// for DevicesIconsColumn
@property(weak) IBOutlet NSImageView* keyboardImage;
@property(weak) IBOutlet NSImageView* mouseImage;

@property KarabinerKitDeviceIdentifiers* deviceIdentifiers;

@end
