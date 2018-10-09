#import "libkrbn.h"
@import Cocoa;

int main(int argc, const char* argv[]) {
  libkrbn_initialize();
  int exit_code = NSApplicationMain(argc, (const char**)argv);
  libkrbn_terminate();
  return exit_code;
}
