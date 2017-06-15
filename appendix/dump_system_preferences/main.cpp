#include "logger.hpp"
#include "system_preferences_monitor.hpp"
#include <iostream>

namespace {
void system_preferences_values_updated_callback(const krbn::system_preferences::values& values) {
  std::cout << "system_preferences_values_updated_callback:" << std::endl;
  std::cout << "  com.apple.keyboard.fnState: " << values.get_keyboard_fn_state() << std::endl;
}
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  krbn::system_preferences_monitor monitor(system_preferences_values_updated_callback);

  CFRunLoopRun();

  return 0;
}
