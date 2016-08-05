#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>

#include <iostream>
#include <memory>
#include <thread>

#include "include_boost.hpp"

#include "connection_manager.hpp"

int main(int argc, const char* argv[]) {
  connection_manager manager;

  CFRunLoopRun();
  return 0;
}
