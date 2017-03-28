#pragma once

#include "constants.hpp"
#include "filesystem.hpp"
#include "spdlog_utility.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

namespace krbn {
class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      auto log_directory = constants::get_user_log_directory();
      if (!log_directory.empty()) {
        filesystem::create_directory_with_intermediate_directories(log_directory, 0700);

        if (filesystem::is_directory(log_directory)) {
          std::string log_file_path = log_directory + "/console_user_server_log";
          logger = spdlog::rotating_logger_mt("console_user_server", log_file_path.c_str(), 256 * 1024, 3);
          logger->flush_on(spdlog::level::info);
          logger->set_pattern(spdlog_utility::get_pattern());
        }
      }

      if (!logger) {
        // fallback
        logger = spdlog::stdout_logger_mt("console_user_server", false);
      }
    }
    return *logger;
  }
};
} // namespace krbn
