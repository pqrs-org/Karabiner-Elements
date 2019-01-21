#import "KarabinerKit/ConfigurationManager.h"
#import "KarabinerKit/JsonUtility.h"
#import "KarabinerKit/NotificationKeys.h"
#import "libkrbn/libkrbn.h"

@interface KarabinerKitConfigurationManager ()

@property(readwrite) KarabinerKitCoreConfigurationModel* coreConfigurationModel;

@end

static void configuration_file_updated_callback(libkrbn_core_configuration* initializedCoreConfiguration,
                                                void* refcon) {
  KarabinerKitConfigurationManager* manager = (__bridge KarabinerKitConfigurationManager*)(refcon);
  if (!manager) {
    return;
  }

  KarabinerKitCoreConfigurationModel* model = [[KarabinerKitCoreConfigurationModel alloc] initWithInitializedCoreConfiguration:initializedCoreConfiguration];
  manager.coreConfigurationModel = model;

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
    libkrbn_enable_configuration_monitor(configuration_file_updated_callback,
                                         (__bridge void*)(self));
  }

  return self;
}

- (void)dealloc {
  libkrbn_disable_configuration_monitor();
}

@end
