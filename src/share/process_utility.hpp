#pragma once

#include "constants.hpp"
#include "filesystem.hpp"
#include <fcntl.h>
#include <sstream>
#include <string>
#include <sys/file.h>

namespace krbn {
class process_utility final {
public:
  static bool lock_single_application(const std::string& pid_file_path) {
    auto pid_directory = filesystem::dirname(pid_file_path);
    if (pid_directory.empty()) {
      throw std::runtime_error("failed to get user pid directory");
    }

    filesystem::create_directory_with_intermediate_directories(pid_directory, 0700);
    if (!filesystem::is_directory(pid_directory)) {
      throw std::runtime_error("failed to create pid directory");
    }

    auto fd = open(pid_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
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
    auto& fd = get_single_application_lock_pid_file_descriptor();
    if (fd >= 0) {
      flock(fd, LOCK_UN);
      close(fd);
      fd = -1;
    }
  }

  static bool lock_single_application_with_user_pid_file(const std::string& pid_file_name) {
    auto pid_directory = constants::get_user_pid_directory();
    if (pid_directory.empty()) {
      throw std::runtime_error("failed to get user pid directory");
    }

    std::string pid_file_path = pid_directory + "/" + pid_file_name;
    return lock_single_application(pid_file_path);
  }

private:
  static int& get_single_application_lock_pid_file_descriptor(void) {
    static int fd = -1;
    return fd;
  }
};
}
