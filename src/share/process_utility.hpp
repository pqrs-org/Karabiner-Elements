#pragma once

#include <fcntl.h>
#include <string>
#include <sys/file.h>

class process_utility final {
public:
  static bool lock_single_application(const std::string& pid_file_path) {
    auto fd = open(pid_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
      throw std::runtime_error("failed to create pid file");
    }

    if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
      return true;
    }

    return false;
  }

  static void unlock_single_application(const std::string& pid_file_path) {
    auto fd = open(pid_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
    if (fd >= 0) {
      flock(fd, LOCK_UN);
    }
  }
};
