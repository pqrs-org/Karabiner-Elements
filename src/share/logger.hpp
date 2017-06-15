#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace krbn {
class logger final {
public:
  static spdlog::logger& get_logger(void) {
    std::lock_guard<std::mutex> guard(get_mutex());

    auto ptr = get_ptr();
    if (ptr) {
      return *ptr;
    }

    return get_stdout_logger();
  }

  static void set_logger(std::shared_ptr<spdlog::logger> logger) {
    std::lock_guard<std::mutex> guard(get_mutex());

    get_ptr() = logger;
  }

private:
  static spdlog::logger& get_stdout_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("karabiner");
    }

    return *logger;
  }

  static std::mutex& get_mutex(void) {
    static std::mutex mutex;
    return mutex;
  }

  static std::shared_ptr<spdlog::logger>& get_ptr(void) {
    static std::shared_ptr<spdlog::logger> ptr;
    return ptr;
  }
};
} // namespace krbn
