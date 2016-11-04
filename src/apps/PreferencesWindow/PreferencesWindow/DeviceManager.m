#import "DeviceManager.h"
#import "JsonUtility.h"
#import "NotificationKeys.h"
#import "libkrbn.h"

@interface DeviceManager ()

@property libkrbn_device_monitor* libkrbn_device_monitor;
@property(copy, readwrite) NSArray<DeviceModel*>* deviceModels;

- (void)loadJsonFile;

@end

static void devices_updated_callback(void* refcon) {
  DeviceManager* manager = (__bridge DeviceManager*)(refcon);
  [manager loadJsonFile];
  [[NSNotificationCenter defaultCenter] postNotificationName:kDevicesAreUpdated object:nil];
}

@implementation DeviceManager

- (void)setup {
  libkrbn_device_monitor* p = NULL;
  if (libkrbn_device_monitor_initialize(&p, devices_updated_callback, (__bridge void*)(self))) {
    return;
  }
  self.libkrbn_device_monitor = p;
}

- (void)dealloc {
  if (self.libkrbn_device_monitor) {
    libkrbn_device_monitor* p = self.libkrbn_device_monitor;
    libkrbn_device_monitor_terminate(&p);
  }
}

- (void)loadJsonFile {
  NSString* filePath = [NSString stringWithUTF8String:libkrbn_get_devices_json_file_path()];
  NSDictionary* jsonObject = [JsonUtility loadFile:filePath];
  if (jsonObject) {
    NSMutableArray* deviceModels = [NSMutableArray new];
    for (NSDictionary* device in jsonObject) {
      DeviceModel* model = [[DeviceModel alloc] initWithDictionary:device];

      BOOL found = NO;
      for (DeviceModel* m in deviceModels) {
        if ([m.deviceIdentifiers isEqualToDeviceIdentifiers:model.deviceIdentifiers]) {
          found = YES;
        }
      }

      if (!found) {
        [deviceModels addObject:model];
      }
    }
    self.deviceModels = [deviceModels sortedArrayUsingComparator:^(DeviceModel* obj1, DeviceModel* obj2) {
      if (obj1.deviceIdentifiers.vendorId != obj2.deviceIdentifiers.vendorId) {
        return obj1.deviceIdentifiers.vendorId < obj2.deviceIdentifiers.vendorId ? NSOrderedAscending : NSOrderedDescending;
      }
      if (obj1.deviceIdentifiers.productId != obj2.deviceIdentifiers.productId) {
        return obj1.deviceIdentifiers.productId < obj2.deviceIdentifiers.productId ? NSOrderedAscending : NSOrderedDescending;
      }
      if (obj1.deviceIdentifiers.isKeyboard != obj2.deviceIdentifiers.isKeyboard) {
        return obj1.deviceIdentifiers.isKeyboard ? NSOrderedAscending : NSOrderedDescending;
      }
      if (obj1.deviceIdentifiers.isPointingDevice != obj2.deviceIdentifiers.isPointingDevice) {
        return obj1.deviceIdentifiers.isPointingDevice ? NSOrderedAscending : NSOrderedDescending;
      }
      return NSOrderedSame;
    }];
  }
}

@end
