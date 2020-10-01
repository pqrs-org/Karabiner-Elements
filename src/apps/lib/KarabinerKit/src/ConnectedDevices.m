#import "KarabinerKit/ConnectedDevices.h"

@interface KarabinerKitConnectedDevices ()

@property libkrbn_connected_devices* libkrbnConnectedDevices;

@end

@implementation KarabinerKitConnectedDevices

- (instancetype)initWithInitializedConnectedDevices:(libkrbn_connected_devices*)initializedConnectedDevices {
  self = [super init];

  if (self) {
    _libkrbnConnectedDevices = initializedConnectedDevices;
  }

  return self;
}

- (void)dealloc {
  if (self.libkrbnConnectedDevices) {
    libkrbn_connected_devices* p = self.libkrbnConnectedDevices;
    if (p) {
      libkrbn_connected_devices_terminate(&p);
    }
  }
}

- (NSUInteger)devicesCount {
  return libkrbn_connected_devices_get_size(self.libkrbnConnectedDevices);
}

- (NSString*)manufacturerAtIndex:(NSUInteger)index {
  NSString* result = @"";

  const char* p = libkrbn_connected_devices_get_descriptions_manufacturer(self.libkrbnConnectedDevices, index);
  if (p) {
    result = [NSString stringWithUTF8String:p];
  }

  if (result.length == 0) {
    result = @"No manufacturer name";
  }

  return result;
}

- (NSString*)productAtIndex:(NSUInteger)index {
  NSString* result = @"";

  const char* p = libkrbn_connected_devices_get_descriptions_product(self.libkrbnConnectedDevices, index);
  if (p) {
    result = [NSString stringWithUTF8String:p];
  }

  if (result.length == 0) {
    result = @"No product name";
  }

  return result;
}

- (libkrbn_device_identifiers)deviceIdentifiersAtIndex:(NSUInteger)index {
  libkrbn_device_identifiers device_identifiers;
  libkrbn_connected_devices_get_device_identifiers(self.libkrbnConnectedDevices, index, &device_identifiers);
  return device_identifiers;
}

- (BOOL)isBuiltInKeyboardAtIndex:(NSUInteger)index {
  return libkrbn_connected_devices_get_is_built_in_keyboard(self.libkrbnConnectedDevices, index);
}

- (BOOL)isBuiltInTrackpadAtIndex:(NSUInteger)index {
  return libkrbn_connected_devices_get_is_built_in_trackpad(self.libkrbnConnectedDevices, index);
}

- (BOOL)isBuiltInTouchBarAtIndex:(NSUInteger)index {
  return libkrbn_connected_devices_get_is_built_in_touch_bar(self.libkrbnConnectedDevices, index);
}

@end
