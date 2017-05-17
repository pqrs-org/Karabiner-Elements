#pragma once

#include "cf_utility.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

class libkrbn final {
public:
  static spdlog::logger& get_logger(void) {
    static std::mutex mutex;
    static std::shared_ptr<spdlog::logger> logger;

    std::lock_guard<std::mutex> guard(mutex);

    if (!logger) {
      logger = spdlog::stdout_logger_mt("libkrbn");
    }
    return *logger;
  }
};
