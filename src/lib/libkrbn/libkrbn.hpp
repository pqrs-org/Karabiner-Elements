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
    std::lock_guard<std::mutex> guard(mutex);

    static std::shared_ptr<spdlog::logger> logger;

    if (!logger) {
      logger = spdlog::stdout_color_mt("libkrbn");
    }
    return *logger;
  }
};
