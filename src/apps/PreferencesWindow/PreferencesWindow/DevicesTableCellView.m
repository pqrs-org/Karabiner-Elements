#import "DevicesTableCellView.h"

@interface DevicesTableCellView ()

@property(readwrite) NSUInteger deviceVendorId;
@property(readwrite) NSUInteger deviceProductId;
@property(readwrite) BOOL deviceIsKeyboard;
@property(readwrite) BOOL deviceIsPointingDevice;
@property(readwrite) NSString* deviceManufacturer;
@property(readwrite) NSString* deviceProduct;

@end

@implementation DevicesTableCellView

- (void)setDeviceIdentifiers:(KarabinerKitConnectedDevices*)connectedDevices index:(NSUInteger)index {
  self.deviceVendorId = [connectedDevices vendorIdAtIndex:index];
  self.deviceProductId = [connectedDevices productIdAtIndex:index];
  self.deviceIsKeyboard = [connectedDevices isKeyboardAtIndex:index];
  self.deviceIsPointingDevice = [connectedDevices isPointingDeviceAtIndex:index];
  self.deviceProduct = [connectedDevices productAtIndex:index];
  self.deviceManufacturer = [connectedDevices manufacturerAtIndex:index];
}

@end
