#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "spdlog.hpp"
#include <deque>
#include <fstream>
#include <vector>

namespace pqrs {
namespace spdlog {
namespace impl {
class merge_log_file final {
public:
  merge_log_file(const ::spdlog::filename_t& file_path) : stream_(file_path) {
    read_next_line();
  }

  const std::string& get_line(void) const {
    return line_;
  }

  const std::optional<uint64_t>& get_sort_key(void) const {
    return sort_key_;
  }

  void read_next_line(void) {
    if (stream_) {
      if (std::getline(stream_, line_)) {
        sort_key_ = spdlog::make_sort_key(line_);

        // skip if sort_key_ == std::nullopt (== broken line)
        if (!sort_key_) {
          line_.clear();
          sort_key_ = 0;
        }

        return;
      }
    }

    line_.clear();
    sort_key_ = std::nullopt;
  }

private:
  std::ifstream stream_;
  std::string line_;
  std::optional<uint64_t> sort_key_;
};
} // namespace impl

inline std::shared_ptr<std::deque<std::string>> read_log_files(const std::vector<::spdlog::filename_t>& target_file_paths,
                                                               size_t max_line_count) {
  auto result = std::make_shared<std::deque<std::string>>();

  std::vector<std::shared_ptr<impl::merge_log_file>> files;
  for (const auto& file_path : target_file_paths) {
    files.push_back(std::make_shared<impl::merge_log_file>(file_path));
    files.push_back(std::make_shared<impl::merge_log_file>(spdlog::make_rotated_file_path(file_path)));
  }

  while (true) {
    auto it = std::min_element(std::begin(files),
                               std::end(files),
                               [](auto&& a, auto&& b) {
                                 if (a->get_sort_key() && b->get_sort_key()) {
                                   return a->get_sort_key() < b->get_sort_key();
                                 }

                                 if (a->get_sort_key()) {
                                   return true;
                                 }

                                 return false;
                               });
    if (it == std::end(files)) {
      break;
    }

    if (!(*it)->get_sort_key()) {
      break;
    }

    if (!(*it)->get_line().empty()) {
      result->push_back((*it)->get_line());
    }

    (*it)->read_next_line();
  }

  if (max_line_count > 0) {
    if (max_line_count < result->size()) {
      result->erase(std::begin(*result),
                    std::begin(*result) + result->size() - max_line_count);
    }
  }

  return result;
}
} // namespace spdlog
} // namespace pqrs
