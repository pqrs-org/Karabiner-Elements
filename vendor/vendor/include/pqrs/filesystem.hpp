#pragma once

// pqrs::filesystem v4.0.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <expected>
#include <fcntl.h>
#include <filesystem>
#include <memory>
#include <optional>
#include <pqrs/gsl.hpp>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

namespace pqrs::filesystem {

[[nodiscard]] inline std::optional<uid_t> uid(const std::filesystem::path& path) noexcept {
  struct stat s;
  if (stat(path.c_str(), &s) == 0) {
    return s.st_uid;
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<uid_t> symlink_uid(const std::filesystem::path& path) noexcept {
  struct stat s;
  if (lstat(path.c_str(), &s) == 0) {
    return s.st_uid;
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<gid_t> gid(const std::filesystem::path& path) noexcept {
  struct stat s;
  if (stat(path.c_str(), &s) == 0) {
    return s.st_gid;
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<gid_t> symlink_gid(const std::filesystem::path& path) noexcept {
  struct stat s;
  if (lstat(path.c_str(), &s) == 0) {
    return s.st_gid;
  }
  return std::nullopt;
}

[[nodiscard]] inline bool is_owned(const std::filesystem::path& path,
                                   uid_t uid) noexcept {
  return pqrs::filesystem::uid(path) == uid;
}

[[nodiscard]] inline bool is_symlink_owned(const std::filesystem::path& path,
                                           uid_t uid) noexcept {
  return pqrs::filesystem::symlink_uid(path) == uid;
}

//
// read_file
//

struct read_file_options final {
  std::optional<size_t> max_size;
  // Unset permits any owner; an empty list permits none.
  std::optional<std::vector<uid_t>> allowed_owners;
};

struct read_file_error final {
  enum class reason {
    open_failed,
    stat_failed,
    read_failed,
    not_regular_file,
    invalid_owner,
    size_limit_exceeded,
  };

  reason type;
  // Preserves errno for OS failures, including EACCES and EPERM.
  std::error_code code;

  [[nodiscard]] std::string message() const {
    switch (type) {
      case reason::open_failed:
        return "open failed: " + code.message();
      case reason::stat_failed:
        return "stat failed: " + code.message();
      case reason::read_failed:
        return "read failed: " + code.message();
      case reason::not_regular_file:
        return "not a regular file";
      case reason::invalid_owner:
        return "not owned by an allowed user";
      case reason::size_limit_exceeded:
        return "file size limit exceeded";
    }
    return "unknown read error";
  }
};

// Follows symlinks, but validates and reads the same opened regular file.
// O_NONBLOCK avoids waiting for a FIFO writer; it does not make disk I/O asynchronous.
// A size limit is checked both before allocation and while reading to EOF.
[[nodiscard]] inline std::expected<not_null_shared_ptr_t<std::vector<uint8_t>>, read_file_error> read_file(
    const std::filesystem::path& path,
    const read_file_options& options = {}) {
  struct file_descriptor final {
    int value;
    ~file_descriptor() {
      if (value >= 0) {
        ::close(value);
      }
    }
  };
  file_descriptor fd{::open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC)};
  if (fd.value < 0) {
    return std::unexpected(read_file_error{
        read_file_error::reason::open_failed,
        {errno, std::generic_category()},
    });
  }

  struct stat status{};
  if (::fstat(fd.value, &status) != 0) {
    return std::unexpected(read_file_error{
        read_file_error::reason::stat_failed,
        {errno, std::generic_category()},
    });
  }

  if (!S_ISREG(status.st_mode)) {
    return std::unexpected(read_file_error{
        read_file_error::reason::not_regular_file,
        {},
    });
  }

  if (options.allowed_owners &&
      !std::ranges::contains(*options.allowed_owners, status.st_uid)) {
    return std::unexpected(read_file_error{
        read_file_error::reason::invalid_owner,
        {},
    });
  }

  std::vector<uint8_t> contents;

  // Check the file size before reserve() to avoid allocating beyond the limit.
  auto limit = std::min(options.max_size.value_or(contents.max_size()),
                        contents.max_size());
  if (!std::in_range<size_t>(status.st_size) ||
      static_cast<size_t>(status.st_size) > limit) {
    return std::unexpected(read_file_error{
        read_file_error::reason::size_limit_exceeded,
        {},
    });
  }

  contents.reserve(static_cast<size_t>(status.st_size));
  std::array<uint8_t, 8192> buffer;
  for (;;) {
    auto size = ::read(fd.value,
                       buffer.data(),
                       buffer.size());
    if (size < 0) {
      auto error = errno;
      if (error == EINTR) {
        // Interrupted by a signal before reading any data; retry the read.
        continue;
      }
      return std::unexpected(read_file_error{
          read_file_error::reason::read_failed,
          {error, std::generic_category()},
      });
    }

    if (size == 0) {
      return not_null_shared_ptr_t<std::vector<uint8_t>>{
          std::make_shared<std::vector<uint8_t>>(std::move(contents)),
      };
    }

    if (static_cast<size_t>(size) > limit - contents.size()) {
      return std::unexpected(read_file_error{
          read_file_error::reason::size_limit_exceeded,
          {},
      });
    }

    contents.insert(contents.end(),
                    buffer.begin(),
                    buffer.begin() + size);
  }
}

//
// write_file_atomically
//

struct write_file_options final {
  std::filesystem::path path;
  std::string_view contents;
  mode_t permissions = 0644;
};

struct write_file_error final {
  enum class reason {
    create_temporary_file_failed,
    write_failed,
    permissions_failed,
    close_failed,
    rename_failed,
  };

  reason type;
  std::error_code code;

  [[nodiscard]] std::string message() const {
    switch (type) {
      case reason::create_temporary_file_failed:
        return "temporary file creation failed: " + code.message();
      case reason::write_failed:
        return "write failed: " + code.message();
      case reason::permissions_failed:
        return "permissions update failed: " + code.message();
      case reason::close_failed:
        return "close failed: " + code.message();
      case reason::rename_failed:
        return "rename failed: " + code.message();
    }
    return "unknown write error";
  }
};

// Writes to a unique temporary file in the destination directory, then atomically
// replaces the destination with rename(2). Existing symlinks at the destination
// are replaced rather than followed.
inline std::expected<void, write_file_error> write_file_atomically(
    const write_file_options& options) {
  struct temporary_file final {
    int file_descriptor;
    std::string path;

    ~temporary_file() {
      if (file_descriptor >= 0) {
        ::close(file_descriptor);
      }
      if (!path.empty()) {
        ::unlink(path.c_str());
      }
    }
  };

  // mkstemp replaces XXXXXX in this writable string buffer with a unique suffix.
  // std::string::data returns a writable buffer since C++17.
  std::string temporary_path = options.path.string() + ".tmp.XXXXXX";

  auto file_descriptor = ::mkostemp(temporary_path.data(),
                                    O_CLOEXEC);
  if (file_descriptor < 0) {
    return std::unexpected(write_file_error{
        write_file_error::reason::create_temporary_file_failed,
        {
            errno,
            std::generic_category(),
        },
    });
  }

  temporary_file file{
      .file_descriptor = file_descriptor,
      .path = temporary_path,
  };

  size_t offset = 0;
  while (offset < options.contents.size()) {
    auto size = ::write(file.file_descriptor,
                        options.contents.data() + offset,
                        options.contents.size() - offset);
    if (size < 0) {
      auto error = errno;
      if (error == EINTR) {
        continue;
      }
      return std::unexpected(write_file_error{
          write_file_error::reason::write_failed,
          {
              error,
              std::generic_category(),
          },
      });
    }

    if (size == 0) {
      return std::unexpected(write_file_error{
          write_file_error::reason::write_failed,
          {
              EIO,
              std::generic_category(),
          },
      });
    }

    offset += static_cast<size_t>(size);
  }

  if (::fchmod(file.file_descriptor,
               options.permissions) != 0) {
    return std::unexpected(write_file_error{
        write_file_error::reason::permissions_failed,
        {
            errno,
            std::generic_category(),
        },
    });
  }

  if (::close(file.file_descriptor) != 0) {
    file.file_descriptor = -1;
    return std::unexpected(write_file_error{
        write_file_error::reason::close_failed,
        {
            errno,
            std::generic_category(),
        },
    });
  }

  file.file_descriptor = -1;

  if (::rename(file.path.c_str(),
               options.path.c_str()) != 0) {
    return std::unexpected(write_file_error{
        write_file_error::reason::rename_failed,
        {
            errno,
            std::generic_category(),
        },
    });
  }

  file.path.clear();

  return {};
}
} // namespace pqrs::filesystem
