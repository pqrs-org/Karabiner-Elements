#pragma once

#include "constants.hpp"
#include "filesystem.hpp"
#include "spdlog_utility.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("event_dispatcher", false);
      logger->flush_on(spdlog::level::info);
      logger->set_pattern(spdlog_utility::get_pattern());
    }
    return *logger;
  }
};
