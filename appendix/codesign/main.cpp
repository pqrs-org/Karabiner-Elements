#include <chrono>
#include <iostream>
#include <thread>

#include "codesign.hpp"
#include <spdlog/spdlog.h>

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("codesign", true);
    }
    return *logger;
  }
};

int main(int argc, const char* argv[]) {
  if (argc > 1) {
    auto common_name = codesign::get_common_name_of_process(atoi(argv[1]));
    logger::get_logger().info("common_name: {0}", common_name ? *common_name : "null");
  }

  return 0;
}
