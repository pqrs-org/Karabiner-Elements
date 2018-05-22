// -*- Mode: objc -*-

@import Cocoa;
#import "KarabinerKit/KarabinerKit.h"

@interface DevicesTableCellView : NSTableCellView

// for DevicesCheckboxColumn
@property(weak) IBOutlet NSButton* checkbox;
// for DevicesKeyboardTypeColumn
@property(weak) IBOutlet NSPopUpButton* popUpButton;
// for DevicesIconsColumn
@property(weak) IBOutlet NSImageView* keyboardImage;
@property(weak) IBOutlet NSImageView* mouseImage;
@property(readwrite) libkrbn_device_identifiers deviceIdentifiers;

@end
