#import "DeviceManager.h"
#import "JsonUtility.h"
#import "NotificationKeys.h"
#import "libkrbn.h"

@interface KarabinerKitDeviceManager ()

@property libkrbn_device_monitor* libkrbn_device_monitor;
@property(copy, readwrite) NSArray<KarabinerKitDeviceModel*>* deviceModels;

- (void)loadJsonFile;

@end

static void devices_updated_callback(void* refcon) {
  KarabinerKitDeviceManager* manager = (__bridge KarabinerKitDeviceManager*)(refcon);
  [manager loadJsonFile];
  [[NSNotificationCenter defaultCenter] postNotificationName:kKarabinerKitDevicesAreUpdated object:nil];
}

@implementation KarabinerKitDeviceManager

+ (KarabinerKitDeviceManager*)sharedManager {
  static dispatch_once_t once;
  static KarabinerKitDeviceManager* manager;
  dispatch_once(&once, ^{
    manager = [KarabinerKitDeviceManager new];

    libkrbn_device_monitor* p = NULL;
    if (libkrbn_device_monitor_initialize(&p, devices_updated_callback, (__bridge void*)(manager))) {
      manager.libkrbn_device_monitor = p;
    }
  });

  return manager;
}

- (void)dealloc {
  if (self.libkrbn_device_monitor) {
    libkrbn_device_monitor* p = self.libkrbn_device_monitor;
    libkrbn_device_monitor_terminate(&p);
  }
}

- (void)loadJsonFile {
  NSString* filePath = [NSString stringWithUTF8String:libkrbn_get_devices_json_file_path()];
  NSDictionary* jsonObject = [KarabinerKitJsonUtility loadFile:filePath];
  if (jsonObject) {
    NSMutableArray* deviceModels = [NSMutableArray new];
    for (NSDictionary* device in jsonObject) {
      KarabinerKitDeviceModel* model = [[KarabinerKitDeviceModel alloc] initWithDictionary:device];

      BOOL found = NO;
      for (KarabinerKitDeviceModel* m in deviceModels) {
        if ([m.deviceIdentifiers isEqualToDeviceIdentifiers:model.deviceIdentifiers]) {
          found = YES;
        }
      }

      if (!found) {
        [deviceModels addObject:model];
      }
    }
    self.deviceModels = [deviceModels sortedArrayUsingComparator:^(KarabinerKitDeviceModel* obj1, KarabinerKitDeviceModel* obj2) {
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
