#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "spdlog.hpp"
#include <algorithm>
#include <cstdint>
#include <deque>
#include <fstream>
#include <pqrs/gsl.hpp>
#include <ranges>
#include <utf8cpp/utf8.h>
#include <vector>

namespace pqrs::spdlog::impl {
class merge_log_file final {
public:
  merge_log_file(const ::spdlog::filename_t& file_path)
      : stream_(file_path) {
    read_next_line();
  }

  [[nodiscard]] const std::string& get_line() const noexcept {
    return line_;
  }

  [[nodiscard]] const std::optional<uint64_t>& get_sort_key() const noexcept {
    return sort_key_;
  }

  void read_next_line() {
    if (stream_) {
      if (std::getline(stream_, line_)) {
        line_ = utf8::replace_invalid(line_);

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
} // namespace pqrs::spdlog::impl

namespace pqrs::spdlog {
[[nodiscard]] inline pqrs::not_null_shared_ptr_t<std::deque<std::string>> read_log_files(const std::vector<::spdlog::filename_t>& target_file_paths,
                                                                                         size_t max_line_count) {
  pqrs::not_null_shared_ptr_t<std::deque<std::string>> result(std::make_shared<std::deque<std::string>>());

  std::vector<not_null_shared_ptr_t<impl::merge_log_file>> files;
  for (const auto& file_path : target_file_paths) {
    files.emplace_back(std::make_shared<impl::merge_log_file>(file_path));
    files.emplace_back(std::make_shared<impl::merge_log_file>(spdlog::make_rotated_file_path(file_path)));
  }

  while (true) {
    auto it = std::ranges::min_element(
        files,
        [](const auto& a, const auto& b) {
          if (a->get_sort_key() && b->get_sort_key()) {
            return a->get_sort_key() < b->get_sort_key();
          }

          return a->get_sort_key().has_value();
        });
    if (it == files.end()) {
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
} // namespace pqrs::spdlog
