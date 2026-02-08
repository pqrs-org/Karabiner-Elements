#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <mutex>
#include <optional>
#include <unistd.h>

namespace pqrs {
namespace process {
class pipe final {
public:
  pipe(void) {
    memset(file_descriptors_, 0, sizeof(file_descriptors_));

    if (::pipe(file_descriptors_) != 0) {
      memset(file_descriptors_, 0, sizeof(file_descriptors_));
    }
  }

  ~pipe(void) {
    close_read_end();
    close_write_end();
  }

  std::optional<int> get_read_end(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    int fd = file_descriptors_[0];
    if (fd) {
      return fd;
    }
    return std::nullopt;
  }

  std::optional<int> get_write_end(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    int fd = file_descriptors_[1];
    if (fd) {
      return fd;
    }
    return std::nullopt;
  }

  void close_read_end(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (file_descriptors_[0]) {
      close(file_descriptors_[0]);
      file_descriptors_[0] = 0;
    }
  }

  void close_write_end(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (file_descriptors_[1]) {
      close(file_descriptors_[1]);
      file_descriptors_[1] = 0;
    }
  }

private:
  int file_descriptors_[2];
  mutable std::mutex mutex_;
};
} // namespace process
} // namespace pqrs
