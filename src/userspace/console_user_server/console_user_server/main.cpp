#include <CoreFoundation/CoreFoundation.h>

#include "include_boost.hpp"

#include "connection_manager.hpp"

int main(int argc, const char* argv[]) {
  connection_manager manager;
  CFRunLoopRun();
  return 0;
}
