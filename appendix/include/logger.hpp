#pragma once

#include <spdlog/spdlog.h>

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_color_mt("log");
    }

    return *logger;
  }
};
