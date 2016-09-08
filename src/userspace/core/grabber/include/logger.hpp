#pragma once

#include "filesystem.hpp"
#include <memory>
#include <spdlog/spdlog.h>

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      auto log_directory = "/var/log/karabiner";
      mkdir(log_directory, 0755);

      if (filesystem::is_directory(log_directory)) {
        logger = spdlog::rotating_logger_mt("grabber", "/var/log/karabiner/grabber_log", 1024 * 1024, 3, true);
      }

      if (!logger) {
        // fallback
        logger = spdlog::stdout_logger_mt("grabber", true);
      }
    }
    return *logger;
  }
};
