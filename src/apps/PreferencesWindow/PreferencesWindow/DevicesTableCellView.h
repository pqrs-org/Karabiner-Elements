// -*- Mode: objc -*-

@import Cocoa;

@interface DevicesTableCellView : NSTableCellView

@property(weak) IBOutlet NSButton* checkbox;
@property uint32_t vendorId;
@property uint32_t productId;

@end
