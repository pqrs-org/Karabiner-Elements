#import "KarabinerKit/DeviceManager.h"
#import "KarabinerKit/NotificationKeys.h"
#import "libkrbn/libkrbn.h"

@interface KarabinerKitDeviceManager ()

@property(readwrite) KarabinerKitConnectedDevices* connectedDevices;

@end

static void connected_devices_updated_callback(libkrbn_connected_devices* initializedConnectedDevices,
                                               void* refcon) {
  KarabinerKitDeviceManager* manager = (__bridge KarabinerKitDeviceManager*)(refcon);
  if (!manager) {
    return;
  }

  KarabinerKitConnectedDevices* devices = [[KarabinerKitConnectedDevices alloc] initWithInitializedConnectedDevices:initializedConnectedDevices];
  manager.connectedDevices = devices;

  [[NSNotificationCenter defaultCenter] postNotificationName:kKarabinerKitDevicesAreUpdated object:nil];
}

@implementation KarabinerKitDeviceManager

+ (KarabinerKitDeviceManager*)sharedManager {
  static dispatch_once_t once;
  static KarabinerKitDeviceManager* manager;
  dispatch_once(&once, ^{
    manager = [KarabinerKitDeviceManager new];
  });

  return manager;
}

- (instancetype)init {
  self = [super init];

  if (self) {
    libkrbn_enable_connected_devices_monitor(connected_devices_updated_callback, (__bridge void*)(self));
  }

  return self;
}

- (void)dealloc {
  libkrbn_disable_connected_devices_monitor();
}

@end
