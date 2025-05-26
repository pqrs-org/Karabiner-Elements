#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <filesystem>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

namespace pqrs {
namespace spdlog {
namespace factory {
inline std::shared_ptr<::spdlog::logger> make_stdout_logger_mt(const std::string& logger_name) {
  return ::spdlog::stdout_logger_mt(logger_name);
}

inline std::shared_ptr<::spdlog::logger> make_async_rotating_logger_mt(const std::string& logger_name,
                                                                       const std::filesystem::path& log_file_path,
                                                                       const std::filesystem::perms& log_directory_perms,
                                                                       std::size_t max_size = 256 * 1024,
                                                                       std::size_t max_files = 3) {
  auto log_directory = log_file_path.parent_path();

  std::error_code error_code;
  std::filesystem::create_directories(log_directory, error_code);
  if (error_code) {
    return nullptr;
  }

  std::filesystem::permissions(log_directory, log_directory_perms, error_code);
  if (error_code) {
    return nullptr;
  }

  return ::spdlog::rotating_logger_mt<::spdlog::async_factory>(logger_name,
                                                               log_file_path.string(),
                                                               max_size,
                                                               max_files);
}
} // namespace factory
} // namespace spdlog
} // namespace pqrs
