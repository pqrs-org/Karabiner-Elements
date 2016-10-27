// -*- Mode: objc -*-

@import Cocoa;
#import "DeviceModel.h"

@interface DevicesTableCellView : NSTableCellView

@property(weak) IBOutlet NSButton* checkbox;
@property(weak) IBOutlet NSPopUpButton* popUpButton;
@property DeviceIdentifiers* deviceIdentifiers;

@end
