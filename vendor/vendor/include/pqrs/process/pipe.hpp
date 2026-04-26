#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <array>
#include <mutex>
#include <optional>
#include <unistd.h>

namespace pqrs::process {
class pipe final {
public:
  pipe() {
    if (::pipe(file_descriptors_.data()) != 0) {
      file_descriptors_.fill(-1);
    }
  }

  ~pipe() {
    close_read_end();
    close_write_end();
  }

  std::optional<int> get_read_end() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (const auto fd = file_descriptors_[0];
        fd != -1) {
      return fd;
    }
    return std::nullopt;
  }

  std::optional<int> get_write_end() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (const auto fd = file_descriptors_[1];
        fd != -1) {
      return fd;
    }
    return std::nullopt;
  }

  void close_read_end() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (file_descriptors_[0] != -1) {
      close(file_descriptors_[0]);
      file_descriptors_[0] = -1;
    }
  }

  void close_write_end() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (file_descriptors_[1] != -1) {
      close(file_descriptors_[1]);
      file_descriptors_[1] = -1;
    }
  }

private:
  std::array<int, 2> file_descriptors_{
      -1,
      -1,
  };
  mutable std::mutex mutex_;
};
} // namespace pqrs::process
