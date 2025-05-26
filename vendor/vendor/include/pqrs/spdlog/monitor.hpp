#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::spdlog::monitor` can be used safely in a multi-threaded environment.

#include "reader.hpp"
#include "spdlog.hpp"
#include <filesystem>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>

namespace pqrs {
namespace spdlog {
class monitor final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(std::shared_ptr<std::deque<std::string>> lines)> log_file_updated;

  // Methods

  monitor(const monitor&) = delete;

  monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
          const std::vector<::spdlog::filename_t>& target_file_paths,
          size_t max_line_count) : dispatcher_client(weak_dispatcher),
                                   target_file_paths_(target_file_paths),
                                   max_line_count_(max_line_count),
                                   timer_(*this) {
  }

  virtual ~monitor(void) {
    detach_from_dispatcher([this] {
      timer_.stop();
    });
  }

  void async_start(std::chrono::milliseconds interval) {
    enqueue_to_dispatcher([this, interval] {
      timer_.start(
          [this] {
            bool updated = false;

            for (const auto& file_path : target_file_paths_) {
              std::error_code error_code;
              auto file_size = std::filesystem::file_size(file_path, error_code);
              if (!error_code) {
                auto it = file_sizes_.find(file_path);
                if (it == std::end(file_sizes_) ||
                    it->second != file_size) {
                  file_sizes_[file_path] = file_size;
                  updated = true;
                }
              }
            }

            if (updated) {
              auto lines = read_log_files(target_file_paths_,
                                          max_line_count_);
              log_file_updated(lines);
            }
          },
          interval);
    });
  }

private:
  std::vector<::spdlog::filename_t> target_file_paths_;
  size_t max_line_count_;
  dispatcher::extra::timer timer_;
  std::unordered_map<std::string, std::uintmax_t> file_sizes_;
};
} // namespace spdlog
} // namespace pqrs
