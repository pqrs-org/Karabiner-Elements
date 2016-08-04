#pragma once

#include <memory>
#include <spdlog/spdlog.h>

class logger {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("karabiner_console_user_server", true);
    }
    return *logger;
  }
};
