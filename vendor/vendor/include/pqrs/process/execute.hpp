#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "process.hpp"
#include <sstream>

namespace pqrs::process {
// Execute the command and wait for it to finish​.
class execute {
public:
  execute(const std::vector<std::string>& argv)
      : time_source_(std::make_shared<pqrs::dispatcher::hardware_time_source>()),
        dispatcher_(std::make_shared<dispatcher::dispatcher>(time_source_)),
        process_(dispatcher_, argv) {
    // `process_.wait()` joins the polling thread, but the signal handlers are
    // invoked on the dispatcher thread. Use `wait` to ensure that the enqueued
    // `stdout_received`, `stderr_received`, `run_failed`, and `exited` handlers
    // have been called before capturing the results.
    const auto wait = pqrs::make_thread_wait();

    std::stringstream stdout;
    std::stringstream stderr;

    process_.stdout_received.connect([&stdout](auto&& buffer) {
      for (const auto& c : *buffer) {
        stdout << c;
      }
    });
    process_.stderr_received.connect([&stderr](auto&& buffer) {
      for (const auto& c : *buffer) {
        stderr << c;
      }
    });
    process_.run_failed.connect([wait] {
      wait->notify();
    });
    process_.exited.connect([this, wait](auto&& status) {
      exit_code_ = WIFEXITED(status) ? std::optional<int>(WEXITSTATUS(status)) : std::nullopt;
      wait->notify();
    });
    process_.run();
    process_.wait();

    wait->wait_notice();

    stdout_ = stdout.str();
    stderr_ = stderr.str();
  }

  ~execute() {
    dispatcher_->terminate();
    dispatcher_ = nullptr;
  }

  const std::string& get_stdout() const {
    return stdout_;
  }

  const std::string& get_stderr() const {
    return stderr_;
  }

  const std::optional<int>& get_exit_code() const {
    return exit_code_;
  }

private:
  std::shared_ptr<dispatcher::hardware_time_source> time_source_;
  std::shared_ptr<dispatcher::dispatcher> dispatcher_;
  process process_;

  std::string stdout_;
  std::string stderr_;
  std::optional<int> exit_code_;
};
} // namespace pqrs::process
