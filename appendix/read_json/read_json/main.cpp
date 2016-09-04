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
  {
    configuration_core configuration(logger::get_logger(), "json/example.json");

    for (const auto& it : configuration.get_current_profile_simple_modifications()) {
      logger::get_logger().info("from:{0} to:{1}",
                                static_cast<uint32_t>(it.first),
                                static_cast<uint32_t>(it.second));
    }
  }
  {
    configuration_core configuration(logger::get_logger(), "json/broken.json");

    for (const auto& it : configuration.get_current_profile_simple_modifications()) {
      logger::get_logger().info("from:{0} to:{1}",
                                static_cast<uint32_t>(it.first),
                                static_cast<uint32_t>(it.second));
    }
  }
  {
    configuration_core configuration(logger::get_logger(), "json/invalid_key_code_name.json");

    for (const auto& it : configuration.get_current_profile_simple_modifications()) {
      logger::get_logger().info("from:{0} to:{1}",
                                static_cast<uint32_t>(it.first),
                                static_cast<uint32_t>(it.second));
    }
  }

  return 0;
}
