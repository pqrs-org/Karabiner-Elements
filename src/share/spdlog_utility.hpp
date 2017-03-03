#pragma once

#include "boost_defs.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <deque>
#include <iomanip>
#include <spdlog/spdlog.h>

namespace krbn {
class spdlog_utility final {
public:
  static std::string get_pattern(void) {
    return "[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v";
  }

  static boost::optional<uint64_t> get_sort_key(const std::string& line) {
    // line == "[2016-09-22 20:18:37.649] [info] [grabber] version 0.90.36"
    // return 20160922201837649

    // We can parse time strictly by using boost::posix_time::time_from_string.
    // But we cannot use it with boost header only.
    // So we use this rough way.

    if (line.size() < strlen("[0000-00-00 00:00:00.000]")) {
      return boost::none;
    }

    if (line.empty()) {
      return boost::none;
    }

    if (line[0] != '[') {
      return boost::none;
    }

    std::string result_string(4 + 2 + 2 +     // years,months,days
                                  2 + 2 + 2 + // hours,minutes,seconds
                                  3,          // milliseconds
                              '0');
    size_t line_pos = 1;
    size_t result_pos = 0;

    // years
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];

    // months
    ++line_pos;
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];

    // days
    ++line_pos;
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];

    // hours
    ++line_pos;
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];

    // minutes
    ++line_pos;
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];

    // seconds
    ++line_pos;
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];

    // milliseconds
    ++line_pos;
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];

    try {
      return boost::lexical_cast<uint64_t>(result_string);
    } catch (...) {
    }
    return boost::none;
  }

  class log_reducer final {
  public:
    log_reducer(const log_reducer&) = delete;

    log_reducer(spdlog::logger& logger) : logger_(logger) {}

    void reset(void) {
      messages_.clear();
    }

    void info(const std::string& message) {
      if (is_ignore(spdlog::level::info, message)) {
        return;
      }

      logger_.info(message);
    }

    void warn(const std::string& message) {
      if (is_ignore(spdlog::level::warn, message)) {
        return;
      }

      logger_.warn(message);
    }

    void error(const std::string& message) {
      if (is_ignore(spdlog::level::err, message)) {
        return;
      }

      logger_.error(message);
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

    spdlog::logger& logger_;

    std::deque<std::pair<spdlog::level::level_enum, std::string>> messages_;
  };
};
}
