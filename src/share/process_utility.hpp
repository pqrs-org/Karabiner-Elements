#pragma once

// `krbn::process_utility` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include <fcntl.h>
#include <mutex>
#include <pqrs/filesystem.hpp>
#include <sstream>
#include <string>
#include <sys/file.h>

namespace krbn {
class process_utility final {
public:
  static bool lock_single_application(const std::filesystem::path& pid_file_path) {
    std::lock_guard<std::mutex> lock(get_mutex());

    auto pid_directory = pid_file_path.parent_path();
    if (pid_directory.empty()) {
      throw std::runtime_error("failed to get user pid directory");
    }

    pqrs::filesystem::create_directory_with_intermediate_directories(pid_directory, 0755);
    if (!pqrs::filesystem::is_directory(pid_directory)) {
      throw std::runtime_error("failed to create pid directory");
    }

    auto fd = open(pid_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
      throw std::runtime_error("failed to create pid file");
    }

    if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
      get_single_application_lock_pid_file_descriptor() = fd;

      std::stringstream ss;
      ss << getpid() << std::endl;
      auto string = ss.str();
      write(fd, string.c_str(), string.size());

      return true;
    }

    return false;
  }

  static void unlock_single_application(void) {
    std::lock_guard<std::mutex> lock(get_mutex());

    auto& fd = get_single_application_lock_pid_file_descriptor();
    if (fd >= 0) {
      flock(fd, LOCK_UN);
      close(fd);
      fd = -1;
    }
  }

  static bool lock_single_application_with_user_pid_file(const std::filesystem::path& pid_file_name) {
    auto pid_directory = constants::get_user_pid_directory();
    if (pid_directory.empty()) {
      throw std::runtime_error("failed to get user pid directory");
    }

    std::filesystem::path pid_file_path = pid_directory / pid_file_name;
    return lock_single_application(pid_file_path);
  }

private:
  static std::mutex& get_mutex(void) {
    static std::mutex mutex;
    return mutex;
  }

  static int& get_single_application_lock_pid_file_descriptor(void) {
    static int fd = -1;
    return fd;
  }
};
} // namespace krbn
