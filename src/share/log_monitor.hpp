#pragma once

#include "file_monitor.hpp"
#include "spdlog_utility.hpp"
#include <fstream>
#include <list>

class log_monitor final {
public:
  log_monitor(const log_monitor&) = delete;

  log_monitor(void) {
    for (const auto& file_path : {
             "/var/log/karabiner/grabber_log.1.txt",
             "/var/log/karabiner/grabber_log.txt",
             "/var/log/karabiner/event_dispatcher_log.txt",
         }) {
      add_lines(file_path);
    }
  }

private:
  void add_lines(const std::string& file_path) {
    std::ifstream stream(file_path);
    std::string line;
    while (std::getline(stream, line)) {
      if (add_line(line)) {
        const size_t max_lines = 250;
        while (lines_.size() > max_lines) {
          lines_.pop_front();
        }
      }
    }
  }

  bool add_line(const std::string& line) {
    if (auto sort_key = spdlog_utility::get_sort_key(line)) {
      if (lines_.empty()) {
        lines_.push_back(std::make_pair(*sort_key, line));
        return true;
      }

      if (*sort_key < lines_.front().first) {
        // line is too old
        return false;
      }

      if (*sort_key > lines_.back().first) {
        lines_.push_back(std::make_pair(*sort_key, line));
        return true;
      }

      for (auto it = lines_.begin(); it != lines_.end(); ++it) {
        if (*sort_key < it->first) {
          lines_.insert(it, std::make_pair(*sort_key, line));
          return true;
        }
      }

      lines_.push_back(std::make_pair(*sort_key, line));
      return true;
    }

    return false;
  }

  std::list<std::pair<uint64_t, std::string>> lines_;
  std::unique_ptr<file_monitor> file_monitor_;
};
