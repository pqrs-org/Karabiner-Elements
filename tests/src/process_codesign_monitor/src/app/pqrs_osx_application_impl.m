#import <AppKit/AppKit.h>

void pqrs_osx_application_finish_launching(void) {
  @autoreleasepool {
    [[NSApplication sharedApplication] finishLaunching];
  }
}
