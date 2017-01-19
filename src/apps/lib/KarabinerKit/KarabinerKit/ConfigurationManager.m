#import "ConfigurationManager.h"
#import "JsonUtility.h"
#import "NotificationKeys.h"
#import "libkrbn.h"

@interface KarabinerKitConfigurationManager ()

@property libkrbn_configuration_monitor* libkrbn_configuration_monitor;
@property(readwrite) KarabinerKitCoreConfigurationModel* coreConfigurationModel;

- (void)loadJsonString:(const char*)jsonString currentProfileJsonString:(const char*)currentProfileJsonString;

@end

static void configuration_file_updated_callback(const char* jsonString, const char* currentProfileJsonString, void* refcon) {
  KarabinerKitConfigurationManager* manager = (__bridge KarabinerKitConfigurationManager*)(refcon);
  [manager loadJsonString:jsonString currentProfileJsonString:currentProfileJsonString];
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

- (void)loadJsonString:(const char*)jsonString currentProfileJsonString:(const char*)currentProfileJsonString {
  NSDictionary* jsonObject = [KarabinerKitJsonUtility loadCString:jsonString];
  NSDictionary* currentProfileJsonObject = [KarabinerKitJsonUtility loadCString:currentProfileJsonString];
  if (jsonObject && currentProfileJsonString) {
    self.coreConfigurationModel = [[KarabinerKitCoreConfigurationModel alloc] initWithProfile:jsonObject currentProfileJsonObject:currentProfileJsonObject];
  }
}

- (void)save {
  NSString* filePath = [NSString stringWithUTF8String:libkrbn_get_core_configuration_file_path()];
  NSDictionary* jsonObject = [KarabinerKitJsonUtility loadFile:filePath];
  if (!jsonObject) {
    jsonObject = @{};
  }
  NSMutableDictionary* mutableJsonObject = [NSMutableDictionary dictionaryWithDictionary:jsonObject];

  if (!mutableJsonObject[@"profiles"]) {
    mutableJsonObject[@"profiles"] = @[];
  }
  NSMutableArray* mutableProfiles = [NSMutableArray arrayWithArray:mutableJsonObject[@"profiles"]];
  NSInteger __block selectedProfileIndex = -1;
  [mutableProfiles enumerateObjectsUsingBlock:^(id obj, NSUInteger index, BOOL* stop) {
    if (obj[@"selected"] && [obj[@"selected"] boolValue]) {
      selectedProfileIndex = (NSInteger)(index);
      *stop = YES;
    }
  }];
  if (selectedProfileIndex == -1) {
    [mutableProfiles addObject:@{
      @"name" : @"Default profile",
      @"selected" : @(YES),
    }];
    selectedProfileIndex = mutableProfiles.count - 1;
  }

  mutableJsonObject[@"global"] = @{
    @"check_for_updates_on_startup" : @(self.coreConfigurationModel.globalConfiguration.checkForUpdatesOnStartup),
    @"show_in_menu_bar" : @(self.coreConfigurationModel.globalConfiguration.showInMenubar),
  };

  NSMutableDictionary* mutableProfile = [NSMutableDictionary dictionaryWithDictionary:mutableProfiles[selectedProfileIndex]];
  mutableProfile[@"simple_modifications"] = self.coreConfigurationModel.simpleModificationsDictionary;
  mutableProfile[@"fn_function_keys"] = self.coreConfigurationModel.fnFunctionKeysDictionary;
  mutableProfile[@"virtual_hid_keyboard"] = self.coreConfigurationModel.virtualHIDKeyboardDictionary;
  mutableProfile[@"devices"] = self.coreConfigurationModel.devicesArray;

  mutableProfiles[selectedProfileIndex] = mutableProfile;

  mutableJsonObject[@"profiles"] = mutableProfiles;

  [KarabinerKitJsonUtility saveJsonToFile:mutableJsonObject filePath:filePath];
}

@end
