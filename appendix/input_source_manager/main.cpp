#include "input_source_manager.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"
#include <Carbon/Carbon.h>

int main(int argc, char** argv) {
  krbn::thread_utility::register_main_thread();

  krbn::input_source_manager input_source_manager;

  CFRunLoopRun();
  return 0;
}
