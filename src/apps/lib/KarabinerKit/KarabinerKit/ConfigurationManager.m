#import "ConfigurationManager.h"
#import "JsonUtility.h"
#import "NotificationKeys.h"
#import "libkrbn.h"

@interface KarabinerKitConfigurationManager ()

@property libkrbn_configuration_monitor* libkrbn_configuration_monitor;
@property(readwrite) KarabinerKitCoreConfigurationModel* coreConfigurationModel;

@end

static void configuration_file_updated_callback(libkrbn_core_configuration* initializedCoreConfiguration,
                                                void* refcon) {
  KarabinerKitConfigurationManager* manager = (__bridge KarabinerKitConfigurationManager*)(refcon);
  manager.coreConfigurationModel = [[KarabinerKitCoreConfigurationModel alloc] initWithInitializedCoreConfiguration:initializedCoreConfiguration];

  [[NSNotificationCenter defaultCenter] postNotificationName:kKarabinerKitConfigurationIsLoaded object:nil];
}

@implementation KarabinerKitConfigurationManager

+ (KarabinerKitConfigurationManager*)sharedManager {
  static dispatch_once_t once;
  static KarabinerKitConfigurationManager* manager;
  dispatch_once(&once, ^{
    manager = [KarabinerKitConfigurationManager new];
  });

  return manager;
}

- (instancetype)init {
  self = [super init];

  if (self) {
    libkrbn_configuration_monitor* p = NULL;
    if (libkrbn_configuration_monitor_initialize(&p, configuration_file_updated_callback, (__bridge void*)(self))) {
      _libkrbn_configuration_monitor = p;
    }
  }

  return self;
}

- (void)dealloc {
  if (self.libkrbn_configuration_monitor) {
    libkrbn_configuration_monitor* p = self.libkrbn_configuration_monitor;
    libkrbn_configuration_monitor_terminate(&p);
  }
}

@end
