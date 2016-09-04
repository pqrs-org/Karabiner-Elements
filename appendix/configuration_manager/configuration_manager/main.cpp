#include "configuration_manager.hpp"

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("configuration_manager", true);
    }
    return *logger;
  }
};

int main(int argc, const char* argv[]) {
  configuration_manager manager(logger::get_logger(), "/Users/tekezo/repo/Karabiner-Elements/appendix/configuration_manager/dot_karabiner_directory");

  CFRunLoopRun();

  return 0;
}
