#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::spdlog::unique_filter` can be used safely in a multi-threaded environment.

namespace pqrs {
namespace spdlog {
class unique_filter final {
public:
  unique_filter(const unique_filter&) = delete;

  unique_filter(std::weak_ptr<::spdlog::logger> weak_logger,
                size_t max_history_count = 16) : weak_logger_(weak_logger),
                                                 max_history_count_(max_history_count) {
  }

  void reset(void) {
    std::lock_guard<std::mutex> lock(messages_mutex_);

    messages_.clear();
  }

  void info(const std::string& message) {
    if (exists(::spdlog::level::info, message)) {
      return;
    }

    if (auto logger = weak_logger_.lock()) {
      logger->info(message);
    }
  }

  void warn(const std::string& message) {
    if (exists(::spdlog::level::warn, message)) {
      return;
    }

    if (auto logger = weak_logger_.lock()) {
      logger->warn(message);
    }
  }

  void error(const std::string& message) {
    if (exists(::spdlog::level::err, message)) {
      return;
    }

    if (auto logger = weak_logger_.lock()) {
      logger->error(message);
    }
  }

private:
  bool exists(::spdlog::level::level_enum level,
              const std::string& message) {
    std::lock_guard<std::mutex> lock(messages_mutex_);

    for (const auto& it : messages_) {
      if (it.first == level && it.second == message) {
        return true;
      }
    }

    messages_.push_back(std::make_pair(level, message));
    while (messages_.size() > max_history_count_) {
      messages_.pop_front();
    }

    return false;
  }

  std::weak_ptr<::spdlog::logger> weak_logger_;
  size_t max_history_count_;
  std::deque<std::pair<::spdlog::level::level_enum, std::string>> messages_;
  mutable std::mutex messages_mutex_;
};
} // namespace spdlog
} // namespace pqrs
