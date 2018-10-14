@import Cocoa;
#import "libkrbn.h"

int main(int argc, char* argv[]) {
  libkrbn_initialize();

  return NSApplicationMain(argc, (const char**)argv);
}
