#pragma once

#include "spdlog_utility.hpp"
#include <memory>
#include <pqrs/filesystem.hpp>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

namespace krbn {
class logger final {
public:
#include "logger/unique_filter.hpp"

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

  static void set_async_rotating_logger(const std::string& logger_name,
                                        const std::string& log_file_path,
                                        mode_t log_directory_mode) {
    auto directory = pqrs::filesystem::dirname(log_file_path);
    pqrs::filesystem::create_directory_with_intermediate_directories(directory, log_directory_mode);
    if (pqrs::filesystem::is_directory(directory)) {
      auto l = spdlog::rotating_logger_mt<spdlog::async_factory>(logger_name,
                                                                 log_file_path,
                                                                 256 * 1024,
                                                                 3);
      l->flush_on(spdlog::level::info);
      l->set_pattern(spdlog_utility::get_pattern());
      set_logger(l);
    }
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
