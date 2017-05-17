#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace krbn {
  class glogger final {
  public:
    static spdlog::logger& get_logger(void) {
      static std::shared_ptr<spdlog::logger> logger;
      if (!logger) {
          logger = spdlog::stdout_logger_mt("general");
      }
      return *logger;
    }
  };
} // namespace krbn
