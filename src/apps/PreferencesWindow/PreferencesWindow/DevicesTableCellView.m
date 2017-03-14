#import "DevicesTableCellView.h"

@interface DevicesTableCellView ()

@property(readwrite) NSUInteger deviceVendorId;
@property(readwrite) NSUInteger deviceProductId;
@property(readwrite) BOOL deviceIsKeyboard;
@property(readwrite) BOOL deviceIsPointingDevice;

@end

@implementation DevicesTableCellView

- (void)setDeviceIdentifiers:(KarabinerKitConnectedDevices*)connectedDevices index:(NSUInteger)index {
  self.deviceVendorId = [connectedDevices vendorIdAtIndex:index];
  self.deviceProductId = [connectedDevices productIdAtIndex:index];
  self.deviceIsKeyboard = [connectedDevices isKeyboardAtIndex:index];
  self.deviceIsPointingDevice = [connectedDevices isPointingDeviceAtIndex:index];
}

@end
