@import Cocoa;
#include "libkrbn.h"

int main(int argc, char* argv[]) {
  libkrbn_initialize();
  int exit_code = NSApplicationMain(argc, (const char**)argv);
  libkrbn_terminate();
  return exit_code;
}
