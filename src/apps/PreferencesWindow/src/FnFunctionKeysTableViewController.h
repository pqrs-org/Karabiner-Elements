// -*- mode: objective-c -*-

@import Cocoa;

@interface FnFunctionKeysTableViewController : NSObject

- (void)setup;
- (void)valueChanged:(id)sender;

- (void)updateConnectedDevicesMenu;
- (NSInteger)selectedConnectedDeviceIndex;

@end
