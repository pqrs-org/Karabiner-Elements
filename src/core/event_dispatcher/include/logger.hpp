#pragma once

#include "constants.hpp"
#include "filesystem.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("event_dispatcher", false);
    }
    return *logger;
  }
};
