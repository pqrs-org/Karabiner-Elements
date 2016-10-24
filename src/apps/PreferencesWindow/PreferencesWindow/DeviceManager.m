#import "DeviceManager.h"
#import "JsonUtility.h"
#import "NotificationKeys.h"
#include "libkrbn.h"

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
      [deviceModels addObject:[[DeviceModel alloc] initWithDevice:device]];
    }
    self.deviceModels = deviceModels;
  }
}

@end
