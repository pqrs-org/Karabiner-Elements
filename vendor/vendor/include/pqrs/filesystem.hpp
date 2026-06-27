#pragma once

// pqrs::filesystem v2.0.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <string>
#include <sys/stat.h>

namespace pqrs::filesystem {

[[nodiscard]] inline std::optional<uid_t> uid(const std::string& path) noexcept {
  struct stat s;
  if (stat(path.c_str(), &s) == 0) {
    return s.st_uid;
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<uid_t> symlink_uid(const std::string& path) noexcept {
  struct stat s;
  if (lstat(path.c_str(), &s) == 0) {
    return s.st_uid;
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<gid_t> gid(const std::string& path) noexcept {
  struct stat s;
  if (stat(path.c_str(), &s) == 0) {
    return s.st_gid;
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<gid_t> symlink_gid(const std::string& path) noexcept {
  struct stat s;
  if (lstat(path.c_str(), &s) == 0) {
    return s.st_gid;
  }
  return std::nullopt;
}

[[nodiscard]] inline bool is_owned(const std::string& path, uid_t uid) noexcept {
  return pqrs::filesystem::uid(path) == uid;
}

[[nodiscard]] inline bool is_symlink_owned(const std::string& path, uid_t uid) noexcept {
  return pqrs::filesystem::symlink_uid(path) == uid;
}

} // namespace pqrs::filesystem
