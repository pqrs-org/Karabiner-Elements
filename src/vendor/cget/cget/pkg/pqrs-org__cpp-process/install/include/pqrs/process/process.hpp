#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "file_actions.hpp"
#include "pipe.hpp"
#include <csignal>
#include <nod/nod.hpp>
#include <optional>
#include <poll.h>
#include <pqrs/dispatcher.hpp>
#include <spawn.h>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

#ifdef __APPLE__
extern char** environ;
#endif

namespace pqrs {
namespace process {
class process final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(std::shared_ptr<std::vector<uint8_t>>)> stdout_received;
  nod::signal<void(std::shared_ptr<std::vector<uint8_t>>)> stderr_received;
  nod::signal<void(void)> run_failed;
  nod::signal<void(int)> exited;

  // Methods

  process(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
          const std::vector<std::string>& argv)
      : dispatcher_client(weak_dispatcher),
        killed_(false) {
    argv_buffer_ = make_argv_buffer(argv);
    argv_ = make_argv(argv_buffer_);

    stdout_pipe_ = std::make_unique<pipe>();
    stderr_pipe_ = std::make_unique<pipe>();
    file_actions_ = make_file_actions(*stdout_pipe_, *stderr_pipe_);
  }

  ~process(void) {
    detach_from_dispatcher([this] {
      kill(SIGKILL);
      wait();

      file_actions_ = nullptr;
      stderr_pipe_ = nullptr;
      stdout_pipe_ = nullptr;
    });
  }

  std::optional<pid_t> get_pid(void) const {
    std::lock_guard<std::mutex> lock(pid_mutex_);

    return pid_;
  }

private:
  void set_pid(std::optional<pid_t> value) {
    std::lock_guard<std::mutex> lock(pid_mutex_);

    pid_ = value;
  }

public:
  void run(void) {
    killed_ = false;

    pid_t pid;
    if (posix_spawn(&pid,
                    argv_[0],
                    file_actions_->get_actions(),
                    nullptr,
                    &(argv_[0]),
                    environ) != 0) {
      enqueue_to_dispatcher([this] {
        run_failed();
      });
      return;
    }

    set_pid(pid);

    stdout_pipe_->close_write_end();
    stderr_pipe_->close_write_end();

    // Start polling thread

    {
      std::lock_guard<std::mutex> lock(thread_mutex_);

      thread_ = std::make_unique<std::thread>([this] {
        std::vector<pollfd> poll_file_descriptors;

        if (auto fd = stdout_pipe_->get_read_end()) {
          poll_file_descriptors.push_back({*fd, POLLIN, 0});
        }
        if (auto fd = stderr_pipe_->get_read_end()) {
          poll_file_descriptors.push_back({*fd, POLLIN, 0});
        }

        if (!poll_file_descriptors.empty()) {
          std::vector<uint8_t> buffer(32 * 1024);
          int timeout = 500;
          while (true) {
            auto poll_result = poll(&(poll_file_descriptors[0]), poll_file_descriptors.size(), timeout);

            if (poll_result < 0) {
              // error
              break;
            } else if (poll_result == 0) {
              // timeout
              if (killed_) {
                break;
              }
              continue;
            }

            int fd = 0;

            if (poll_file_descriptors[0].revents & POLLIN) {
              fd = poll_file_descriptors[0].fd;

            } else if (poll_file_descriptors[1].revents & POLLIN) {
              fd = poll_file_descriptors[1].fd;

            } else {
              break;
            }

            auto n = read(fd, &(buffer[0]), buffer.size());
            if (n > 0) {
              auto b = std::make_shared<std::vector<uint8_t>>(std::begin(buffer), std::begin(buffer) + n);

              if (fd == poll_file_descriptors[0].fd) {
                enqueue_to_dispatcher([this, b] {
                  stdout_received(b);
                });
              } else if (fd == poll_file_descriptors[1].fd) {
                enqueue_to_dispatcher([this, b] {
                  stderr_received(b);
                });
              }
            } else {
              break;
            }
          }
        }

        // Wait process

        if (auto pid = get_pid()) {
          int stat;
          if (waitpid(*pid, &stat, 0) == *pid) {
            set_pid(std::nullopt);

            enqueue_to_dispatcher([this, stat] {
              exited(stat);
            });
          }
        }
      });
    }
  }

  void kill(int signal) {
    killed_ = true;

    if (auto pid = get_pid()) {
      ::kill(*pid, signal);
    }
  }

  void wait(void) {
    std::shared_ptr<std::thread> t;

    {
      std::lock_guard<std::mutex> lock(thread_mutex_);

      t = thread_;
    }

    if (t && t->joinable()) {
      t->join();
    }
  }

private:
  static std::vector<std::vector<char>> make_argv_buffer(const std::vector<std::string>& argv) {
    std::vector<std::vector<char>> buffer;

    for (const auto& a : argv) {
      std::vector<char> b(std::begin(a), std::end(a));
      b.push_back('\0');
      buffer.push_back(b);
    }

    return buffer;
  }

  static std::vector<char*> make_argv(std::vector<std::vector<char>>& buffer) {
    std::vector<char*> argv;

    for (auto&& b : buffer) {
      argv.push_back(&(b[0]));
    }

    argv.push_back(nullptr);

    return argv;
  }

  static std::unique_ptr<file_actions> make_file_actions(const pipe& stdout_pipe,
                                                         const pipe& stderr_pipe) {
    auto actions = std::make_unique<file_actions>();

    if (auto fd = stdout_pipe.get_read_end()) {
      actions->addclose(*fd);
    }

    if (auto fd = stdout_pipe.get_write_end()) {
      actions->adddup2(*fd, 1);
      actions->addclose(*fd);
    }

    if (auto fd = stderr_pipe.get_read_end()) {
      actions->addclose(*fd);
    }

    if (auto fd = stderr_pipe.get_write_end()) {
      actions->adddup2(*fd, 2);
      actions->addclose(*fd);
    }

    return actions;
  }

  std::vector<std::vector<char>> argv_buffer_;
  std::vector<char*> argv_;

  std::unique_ptr<pipe> stdout_pipe_;
  std::unique_ptr<pipe> stderr_pipe_;
  std::unique_ptr<file_actions> file_actions_;

  std::optional<pid_t> pid_;
  mutable std::mutex pid_mutex_;

  std::shared_ptr<std::thread> thread_;
  mutable std::mutex thread_mutex_;

  std::atomic<bool> killed_;
};
} // namespace process
} // namespace pqrs
