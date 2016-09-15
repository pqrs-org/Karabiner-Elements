#pragma once

#include "constants.hpp"
#include "logger.hpp"
#include <cstdlib>
#include <thread>
#include <unistd.h>

class event_dispatcher_manager final {
public:
  event_dispatcher_manager(void) : exit_loop_(false), pid_(0) {
    // kill orphan karabiner_event_dispatcher
    system("/usr/bin/killall karabiner_event_dispatcher");

    thread_ = std::thread([this] { this->worker(); });
  }

  ~event_dispatcher_manager(void) {
    exit_loop_ = true;
    if (pid_ > 0) {
      kill(pid_, SIGTERM);
    }
    thread_.join();
  }

private:
  void worker(void) {
    while (!exit_loop_) {
      std::this_thread::sleep_for(std::chrono::seconds(1));

      auto pid = fork();
      if (pid == 0) {
        // redirect stdout to log file
        int log_file_fd = open("/var/log/karabiner/event_dispatcher_log.txt", O_WRONLY | O_CREAT | O_TRUNC);
        if (log_file_fd >= 0) {
          dup2(log_file_fd, 1);
        }

        auto path = constants::get_event_dispatcher_binary_file_path();
        execl(path, path, nullptr);
        exit(0);
      }
      if (pid > 0) {
        logger::get_logger().info("karabiner_event_dispatcher is launched. (pid:{0})", pid);
        pid_ = pid;
        int status;
        waitpid(pid, &status, 0);
        pid_ = 0;
        logger::get_logger().info("karabiner_event_dispatcher is terminated. (pid:{0})", pid);
      }
    }
  }

  std::thread thread_;
  std::atomic<bool> exit_loop_;

  std::atomic<pid_t> pid_;
};
