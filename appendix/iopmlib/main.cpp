#include "iopm_client.hpp"

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("iopmlib", true);
    }
    return *logger;
  }
};

static void callback(uint32_t message_type) {
  logger::get_logger().info("callback");
}

int main(int argc, const char* argv[]) {
  thread_utility::register_main_thread();

  iopm_client client(logger::get_logger(), std::bind(callback, std::placeholders::_1));

  CFRunLoopRun();

  return 0;
}
