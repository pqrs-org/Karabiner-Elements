#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::spdlog::monitor` can be used safely in a multi-threaded environment.

#include "reader.hpp"
#include "spdlog.hpp"
#include <filesystem>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>

namespace pqrs::spdlog {
class monitor final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(pqrs::not_null_shared_ptr_t<std::deque<std::string>> lines)> log_file_updated;

  // Methods

  monitor(const monitor&) = delete;

  monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
          const std::vector<::spdlog::filename_t>& target_file_paths,
          size_t max_line_count) : dispatcher_client(weak_dispatcher),
                                   target_file_paths_(target_file_paths),
                                   max_line_count_(max_line_count),
                                   timer_(*this) {
  }

  ~monitor() override {
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
              for (const auto& monitored_file_path : {
                       file_path,
                       spdlog::make_rotated_file_path(file_path),
                   }) {
                // Log files are expected to be append-only, so file size is enough to detect updates.
                std::error_code error_code;
                auto file_size = std::filesystem::file_size(monitored_file_path, error_code);
                if (!error_code) {
                  if (auto it = file_sizes_.find(monitored_file_path);
                      it == file_sizes_.end() ||
                      it->second != file_size) {
                    file_sizes_[monitored_file_path] = file_size;
                    updated = true;
                  }
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
} // namespace pqrs::spdlog
