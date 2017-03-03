#include "boost_defs.hpp"

#include <iostream>

#include "version_monitor.hpp"

namespace {
class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("version_monitor", true);
    }
    return *logger;
  }
};
}

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  krbn::version_monitor monitor(logger::get_logger(), [] {
    logger::get_logger().info("version_changed_callback");
  });

  CFRunLoopRun();
  return 0;
}
