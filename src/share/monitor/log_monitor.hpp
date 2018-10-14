#pragma once

#include "boost_defs.hpp"

#include "dispatcher.hpp"
#include "filesystem.hpp"
#include "spdlog_utility.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <boost/signals2.hpp>
#include <deque>
#include <fstream>
#include <thread>
#include <type_safe/strong_typedef.hpp>
#include <vector>

namespace krbn {
class log_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  boost::signals2::signal<void(const std::string& line)> new_log_line_arrived;

  // Methods

  log_monitor(const log_monitor&) = delete;

  // FSEvents (file_monitor) notifies file changes only when the target file is just closed.
  // So, it is not usable for log_monitor since spdlog keeps opening log files and appending lines.
  //
  // We use timer to observe file changes instead.

  log_monitor(const std::vector<std::string>& targets) : dispatcher_client(),
                                                         timer_(*this),
                                                         timer_count_(0) {
    // setup initial_lines_

    for (const auto& target : targets) {
      add_initial_lines(target + ".1");
      add_initial_lines(target);

      files_.push_back(target);
    }
  }

  virtual ~log_monitor(void) {
    detach_from_dispatcher([this] {
      timer_.stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      timer_.start(
          [this] {
            ++timer_count_;
            for (const auto& file : files_) {
              if (auto size = filesystem::file_size(file)) {
                auto it = read_position_.find(file);
                if (it != read_position_.end()) {
                  if (it->second != *size) {
                    add_lines(file);
                  }
                }
              }
            }
            call_slots();
          },
          std::chrono::milliseconds(1000));
    });
  }

  const std::vector<std::pair<uint64_t, std::string>>& get_initial_lines(void) const {
    return initial_lines_;
  }

private:
  struct timer_count : type_safe::strong_typedef<timer_count, uint64_t>,
                       type_safe::strong_typedef_op::equality_comparison<timer_count>,
                       type_safe::strong_typedef_op::relational_comparison<timer_count>,
                       type_safe::strong_typedef_op::integer_arithmetic<timer_count> {
    using strong_typedef::strong_typedef;
  };

  void add_initial_lines(const std::string& file_path) {
    std::ifstream stream(file_path);
    std::string line;
    std::streampos read_position;

    while (std::getline(stream, line)) {
      if (add_initial_line(line)) {
        const size_t max_initial_lines = 250;
        while (initial_lines_.size() > max_initial_lines) {
          initial_lines_.erase(initial_lines_.begin());
        }
      }
      read_position = stream.tellg();
    }

    read_position_[file_path] = read_position;
  }

  bool add_initial_line(const std::string& line) {
    if (auto sort_key = spdlog_utility::get_sort_key(line)) {
      if (initial_lines_.empty()) {
        initial_lines_.push_back(std::make_pair(*sort_key, line));
        return true;
      }

      if (*sort_key < initial_lines_.front().first) {
        // line is too old
        return false;
      }

      if (*sort_key > initial_lines_.back().first) {
        initial_lines_.push_back(std::make_pair(*sort_key, line));
        return true;
      }

      for (auto it = initial_lines_.begin(); it != initial_lines_.end(); ++it) {
        if (*sort_key < it->first) {
          initial_lines_.insert(it, std::make_pair(*sort_key, line));
          return true;
        }
      }

      initial_lines_.push_back(std::make_pair(*sort_key, line));
      return true;
    }

    return false;
  }

  void add_lines(const std::string& file_path) {
    std::ifstream stream(file_path);
    if (!stream) {
      return;
    }

    // ----------------------------------------
    // seek

    auto it = read_position_.find(file_path);
    if (it != read_position_.end()) {
      if (auto size = filesystem::file_size(file_path)) {
        if (it->second < *size) {
          stream.seekg(it->second);
        }
      }
    }

    // ----------------------------------------
    // read

    std::string line;
    std::streampos read_position;
    while (std::getline(stream, line)) {
      if (auto sort_key = spdlog_utility::get_sort_key(line)) {
        added_lines_.push_back(std::make_tuple(timer_count_, *sort_key, line));
      }
      read_position = stream.tellg();
    }

    read_position_[file_path] = read_position;

    // ----------------------------------------
    // sort

    std::stable_sort(added_lines_.begin(), added_lines_.end(), [](const auto& a, const auto& b) {
      return std::get<1>(a) < std::get<1>(b);
    });
  }

  void call_slots(void) {
    while (true) {
      if (added_lines_.empty()) {
        return;
      }

      auto front = added_lines_.front();

      if (std::get<0>(front) != timer_count_) {
        // Wait if front is just added.
        return;
      }

      auto line = std::get<2>(front);
      enqueue_to_dispatcher([this, line] {
        new_log_line_arrived(line);
      });

      added_lines_.pop_front();
    }
  }

  pqrs::dispatcher::extra::timer timer_;
  timer_count timer_count_;

  std::vector<std::pair<uint64_t, std::string>> initial_lines_;
  std::unordered_map<std::string, std::streampos> read_position_;
  std::vector<std::string> files_;
  std::deque<std::tuple<timer_count, uint64_t, std::string>> added_lines_;
};
} // namespace krbn
