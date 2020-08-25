#pragma once

#include <deque>
#include <memory>
#include <pqrs/filesystem.hpp>
#include <pqrs/spdlog.hpp>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>

namespace krbn {
class logger final {
public:
  static std::shared_ptr<spdlog::logger> get_logger(void) {
    std::lock_guard<std::mutex> guard(mutex_);

    if (logger_) {
      return logger_;
    }

    //
    // Fallback
    //

    static std::shared_ptr<spdlog::logger> stdout_logger;
    if (!stdout_logger) {
      stdout_logger = pqrs::spdlog::factory::make_stdout_logger_mt("karabiner");
    }

    return stdout_logger;
  }

  static void set_async_rotating_logger(const std::string& logger_name,
                                        const std::filesystem::path& log_file_path,
                                        const std::filesystem::perms& log_directory_perms) {
    auto l = pqrs::spdlog::factory::make_async_rotating_logger_mt(logger_name,
                                                                  log_file_path,
                                                                  log_directory_perms,
                                                                  256 * 1024,
                                                                  3);
    if (l) {
      l->flush_on(spdlog::level::info);
      l->set_pattern(pqrs::spdlog::get_pattern());

      std::lock_guard<std::mutex> guard(mutex_);
      logger_ = l;
    }
  }

  static void set_stdout_color_logger(const std::string& logger_name,
                                      const std::string& pattern) {
    auto l = spdlog::stdout_color_mt(logger_name);
    l->set_level(spdlog::level::err);
    l->set_pattern(pattern);

    std::lock_guard<std::mutex> guard(mutex_);
    logger_ = l;
  }

private:
  static inline std::mutex mutex_;
  static inline std::shared_ptr<spdlog::logger> logger_;
}; // namespace krbn
} // namespace krbn
