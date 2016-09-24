#include "boost_defs.hpp"

#include <iostream>

#include "log_monitor.hpp"

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("log_monitor", true);
    }
    return *logger;
  }
};

int main(int argc, const char* argv[]) {
  log_monitor d;
  CFRunLoopRun();
  return 0;
}
