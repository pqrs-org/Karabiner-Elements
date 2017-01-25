#import "ConfigurationManager.h"
#import "JsonUtility.h"
#import "NotificationKeys.h"
#import "libkrbn.h"

@interface KarabinerKitConfigurationManager ()

@property libkrbn_configuration_monitor* libkrbn_configuration_monitor;
@property(readwrite) KarabinerKitCoreConfigurationModel* coreConfigurationModel;

- (void)loadJsonString:(const char*)jsonString;

@end

static void configuration_file_updated_callback(const char* jsonString, void* refcon) {
  KarabinerKitConfigurationManager* manager = (__bridge KarabinerKitConfigurationManager*)(refcon);
  [manager loadJsonString:jsonString];
  [[NSNotificationCenter defaultCenter] postNotificationName:kKarabinerKitConfigurationIsLoaded object:nil];
}

@implementation KarabinerKitConfigurationManager

+ (KarabinerKitConfigurationManager*)sharedManager {
  static dispatch_once_t once;
  static KarabinerKitConfigurationManager* manager;
  dispatch_once(&once, ^{
    manager = [KarabinerKitConfigurationManager new];

    libkrbn_configuration_monitor* p = NULL;
    if (libkrbn_configuration_monitor_initialize(&p, configuration_file_updated_callback, (__bridge void*)(manager))) {
      return;
    }
    manager.libkrbn_configuration_monitor = p;
  });

  return manager;
}

- (void)dealloc {
  if (self.libkrbn_configuration_monitor) {
    libkrbn_configuration_monitor* p = self.libkrbn_configuration_monitor;
    libkrbn_configuration_monitor_terminate(&p);
  }
}

- (void)loadJsonString:(const char*)jsonString {
  NSDictionary* jsonObject = [KarabinerKitJsonUtility loadCString:jsonString];
  if (jsonObject) {
    self.coreConfigurationModel = [[KarabinerKitCoreConfigurationModel alloc] initWithJsonObject:jsonObject];
  }
}

- (void)save {
  NSString* filePath = [NSString stringWithUTF8String:libkrbn_get_core_configuration_file_path()];
  NSDictionary* jsonObject = [KarabinerKitJsonUtility loadFile:filePath];
  if (!jsonObject) {
    jsonObject = @{};
  }
  NSMutableDictionary* mutableJsonObject = [jsonObject mutableCopy];

  mutableJsonObject[@"global"] = @{
    @"check_for_updates_on_startup" : @(self.coreConfigurationModel.globalConfiguration.checkForUpdatesOnStartup),
    @"show_in_menu_bar" : @(self.coreConfigurationModel.globalConfiguration.showInMenubar),
  };

  NSMutableArray* profiles = [NSMutableArray new];
  for (KarabinerKitConfigurationProfile* profile in self.coreConfigurationModel.profiles) {
    [profiles addObject:profile.jsonObject];
  }
  mutableJsonObject[@"profiles"] = profiles;

  [KarabinerKitJsonUtility saveJsonToFile:mutableJsonObject filePath:filePath];
}

@end
