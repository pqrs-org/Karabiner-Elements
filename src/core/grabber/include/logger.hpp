#pragma once

#include "filesystem.hpp"
#include "spdlog_utility.hpp"
#include <memory>
#include <spdlog/spdlog.h>

namespace krbn {
class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      auto log_directory = "/var/log/karabiner";
      mkdir(log_directory, 0755);

      if (filesystem::is_directory(log_directory)) {
        logger = spdlog::rotating_logger_mt("grabber", "/var/log/karabiner/grabber.log", 256 * 1024, 3);
        logger->flush_on(spdlog::level::info);
        logger->set_pattern(spdlog_utility::get_pattern());
      }

      if (!logger) {
        // fallback
        logger = spdlog::stdout_logger_mt("grabber");
      }
    }
    return *logger;
  }
};
} // namespace krbn
