#import "ConfigurationManager.h"
#include "libkrbn.h"

@interface ConfigurationManager ()

@property libkrbn_configuration_monitor* libkrbn_configuration_monitor;
@property(copy, readwrite) NSDictionary* currentProfile;

- (void)loadJsonString:(const char*)currentProfileJsonString;

@end

static void configuration_file_updated_callback(const char* currentProfileJsonString, void* refcon) {
  ConfigurationManager* manager = (__bridge ConfigurationManager*)(refcon);
  [manager loadJsonString:currentProfileJsonString];
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

- (void)loadJsonString:(const char*)currentProfileJsonString {
  if (!currentProfileJsonString) {
    NSLog(@"currentProfileJsonString is null @ loadJsonString");
    return;
  }

  // Do not include the last '\0' to data. (set length == strlen)
  NSData* data = [NSData dataWithBytesNoCopy:(void*)(currentProfileJsonString)
                                      length:strlen(currentProfileJsonString)
                                freeWhenDone:NO];

  NSError* error = nil;
  NSDictionary* jsonObject = [NSJSONSerialization JSONObjectWithData:data
                                                             options:0
                                                               error:&error];
  if (error) {
    NSLog(@"JSONObjectWithStream error @ loadJsonFile: %@", error);
    return;
  }

  self.currentProfile = jsonObject;
}

@end
