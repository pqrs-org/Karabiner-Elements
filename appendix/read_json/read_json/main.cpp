#include "configuration_core.hpp"
#include <iostream>
#include <spdlog/spdlog.h>

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("read_json", true);
    }
    return *logger;
  }
};

int main(int argc, const char* argv[]) {
  configuration_core configuration(logger::get_logger());

  if (auto k = krbn::types::get_key_code("caps_lock")) {
    logger::get_logger().info("caps_lock {0}", static_cast<uint32_t>(*k));
  }

  return 0;
}
