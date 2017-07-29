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
@property(readonly) NSUInteger deviceVendorId;
@property(readonly) NSUInteger deviceProductId;
@property(readonly) BOOL deviceIsKeyboard;
@property(readonly) BOOL deviceIsPointingDevice;
@property(readonly) NSString* deviceManufacturer;
@property(readonly) NSString* deviceProduct;


- (void)setDeviceIdentifiers:(KarabinerKitConnectedDevices*)connectedDevices index:(NSUInteger)index;

@end
