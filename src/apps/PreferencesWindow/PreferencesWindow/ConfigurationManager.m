#import "ConfigurationManager.h"
#include "libkrbn.h"

@interface ConfigurationManager ()

@property libkrbn_configuration_monitor* libkrbn_configuration_monitor;

- (void)loadJsonFile:(const char*)filePath;

@end

static void configuration_file_updated_callback(const char* filePath, void* refcon) {
  ConfigurationManager* manager = (__bridge ConfigurationManager*)(refcon);
  [manager loadJsonFile:filePath];
}

@implementation ConfigurationManager

- (void)setup {
  libkrbn_configuration_monitor* p = NULL;
  if (libkrbn_configuration_monitor_initialize(&p, configuration_file_updated_callback, (__bridge void*)(self))) {
    return;
  }
  self.libkrbn_configuration_monitor = p;
}

- (void)dealloc {
  if (self.libkrbn_configuration_monitor) {
    libkrbn_configuration_monitor* p = self.libkrbn_configuration_monitor;
    libkrbn_configuration_monitor_terminate(&p);
  }
}

- (void)loadJsonFile:(const char*)filePath {
  NSLog(@"filePath %s", filePath);
}

@end
