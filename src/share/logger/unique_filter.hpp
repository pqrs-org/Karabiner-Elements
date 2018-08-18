#pragma once

class unique_filter final {
public:
  unique_filter(const unique_filter&) = delete;

  unique_filter(void) {
  }

  void reset(void) {
    messages_.clear();
  }

  void info(const std::string& message) {
    if (is_ignore(spdlog::level::info, message)) {
      return;
    }

    logger::get_logger().info(message);
  }

  void warn(const std::string& message) {
    if (is_ignore(spdlog::level::warn, message)) {
      return;
    }

    logger::get_logger().warn(message);
  }

  void error(const std::string& message) {
    if (is_ignore(spdlog::level::err, message)) {
      return;
    }

    logger::get_logger().error(message);
  }

private:
  bool is_ignore(spdlog::level::level_enum level, const std::string& message) {
    for (const auto& it : messages_) {
      if (it.first == level && it.second == message) {
        return true;
      }
    }

    const size_t max_size = 16;
    messages_.push_back(std::make_pair(level, message));
    while (messages_.size() > max_size) {
      messages_.pop_front();
    }

    return false;
  }

  std::deque<std::pair<spdlog::level::level_enum, std::string>> messages_;
};
