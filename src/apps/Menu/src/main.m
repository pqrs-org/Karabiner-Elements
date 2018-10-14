@import Cocoa;
#import "libkrbn.h"

int main(int argc, const char* argv[]) {
  libkrbn_initialize();

  return NSApplicationMain(argc, (const char**)argv);
}
