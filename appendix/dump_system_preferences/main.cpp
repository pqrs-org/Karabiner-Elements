// OS X headers
#include "system_preferences_monitor.hpp"
#include <iostream>

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("dump_system_preferences", true);
    }
    return *logger;
  }
};

namespace {
void system_preferences_values_updated_callback(const system_preferences::values& values) {
  std::cout << "system_preferences_values_updated_callback:" << std::endl;
  std::cout << "  com.apple.keyboard.fnState: " << values.get_keyboard_fn_state() << std::endl;
  std::cout << "  InitialKeyRepeat: " << values.get_initial_key_repeat_milliseconds() << std::endl;
  std::cout << "  KeyRepeat: " << values.get_key_repeat_milliseconds() << std::endl;
}
}

int main(int argc, const char* argv[]) {
  system_preferences_monitor monitor(logger::get_logger(), system_preferences_values_updated_callback);

  CFRunLoopRun();

  return 0;
}
