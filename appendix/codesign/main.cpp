#include "codesign.hpp"
#include "thread_utility.hpp"
#include <chrono>
#include <iostream>
#include <spdlog/spdlog.h>
#include <thread>

namespace {
class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_color_mt("codesign");
    }
    return *logger;
  }
};
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  if (argc > 1) {
    auto common_name = krbn::codesign::get_common_name_of_process(atoi(argv[1]));
    logger::get_logger().info("common_name: {0}", common_name ? *common_name : "null");
  }

  return 0;
}
