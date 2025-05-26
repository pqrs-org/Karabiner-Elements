#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "process.hpp"
#include <sstream>

namespace pqrs {
namespace process {
// Execute the command and wait for it to finish​.
class execute {
public:
  execute(const std::vector<std::string>& argv)
      : time_source_(std::make_shared<pqrs::dispatcher::hardware_time_source>()),
        dispatcher_(std::make_shared<dispatcher::dispatcher>(time_source_)),
        process_(dispatcher_, argv) {
    // Use `wait` to ensure that all enqueued `stdout_received` and `stderr_received` are called.
    auto wait = pqrs::make_thread_wait();

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
      exit_code_ = status;
      wait->notify();
    });
    process_.run();
    process_.wait();

    wait->wait_notice();

    stdout_ = stdout.str();
    stderr_ = stderr.str();
  }

  ~execute(void) {
    dispatcher_->terminate();
    dispatcher_ = nullptr;
  }

  const std::string& get_stdout(void) const {
    return stdout_;
  }

  const std::string& get_stderr(void) const {
    return stderr_;
  }

  const std::optional<int>& get_exit_code(void) const {
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
} // namespace process
} // namespace pqrs
