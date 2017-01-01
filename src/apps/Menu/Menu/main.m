#import "libkrbn.h"
@import Cocoa;

int main(int argc, const char* argv[]) {
  libkrbn_initialize();
  return NSApplicationMain(argc, argv);
}
