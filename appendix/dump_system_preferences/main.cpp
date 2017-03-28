// OS X headers
#include "system_preferences_monitor.hpp"
#include <iostream>

namespace {
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

void system_preferences_values_updated_callback(const krbn::system_preferences::values& values) {
  std::cout << "system_preferences_values_updated_callback:" << std::endl;
  std::cout << "  com.apple.keyboard.fnState: " << values.get_keyboard_fn_state() << std::endl;
}
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  krbn::system_preferences_monitor monitor(logger::get_logger(), system_preferences_values_updated_callback);

  CFRunLoopRun();

  return 0;
}
