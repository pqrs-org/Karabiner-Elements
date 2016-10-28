#import "libkrbn.h"
#import <Cocoa/Cocoa.h>

int main(int argc, const char* argv[]) {
  libkrbn_initialize();
  return NSApplicationMain(argc, argv);
}
