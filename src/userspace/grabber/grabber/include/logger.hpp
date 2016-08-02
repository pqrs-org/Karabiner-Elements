#pragma once

#include <memory>
#include <spdlog/spdlog.h>

class logger {
public:
  static std::shared_ptr<spdlog::logger> get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::rotating_logger_mt("karabiner_grabber", "karabiner_grabber_log", 5 * 1024 * 1024, 3, true);
    }
    return logger;
  }
};
