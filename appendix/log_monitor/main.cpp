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

namespace {
  void new_log_line_callback(const std::string& line) {
    std::cout << line << std::endl;
  }
}

int main(int argc, const char* argv[]) {
  std::vector<std::string> targets = {
    "/var/log/karabiner/grabber_log",
    "/var/log/karabiner/event_dispatcher_log",
  };
  log_monitor monitor(logger::get_logger(), targets, new_log_line_callback);

#if 1
  for (const auto& it : monitor.get_initial_lines()) {
    std::cout << it.second << std::endl;
  }
#endif

  monitor.start();

  CFRunLoopRun();
  return 0;
}
