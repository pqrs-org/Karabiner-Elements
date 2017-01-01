#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

class libkrbn {
public:
  static bool cfstring_to_cstring(std::vector<char>& v, CFStringRef string) {
    if (string) {
      if (auto cstring = CFStringGetCStringPtr(string, kCFStringEncodingUTF8)) {
        auto length = strlen(cstring) + 1;
        v.resize(length);
        strlcpy(&(v[0]), cstring, length);
        return true;
      }
    }

    v.resize(1);
    v[0] = '\0';
    return false;
  }

  static spdlog::logger& get_logger(void) {
    static std::mutex mutex;
    static std::shared_ptr<spdlog::logger> logger;

    std::lock_guard<std::mutex> guard(mutex);

    if (!logger) {
      logger = spdlog::stdout_logger_mt("libkrbn", true);
    }
    return *logger;
  }
};
