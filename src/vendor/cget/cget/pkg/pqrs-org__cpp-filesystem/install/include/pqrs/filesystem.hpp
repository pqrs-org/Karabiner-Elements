#pragma once

// pqrs::filesystem v1.1

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "filesystem/impl.hpp"
#include <array>
#include <climits>
#include <fstream>
#include <optional>
#include <sys/stat.h>
#include <vector>

namespace pqrs {
namespace filesystem {
inline bool exists(const std::string& path) {
  struct stat s;
  return (stat(path.c_str(), &s) == 0);
}

inline bool is_directory(const std::string& path) {
  struct stat s;
  if (stat(path.c_str(), &s) == 0) {
    return S_ISDIR(s.st_mode);
  }
  return false;
}

inline bool is_owned(const std::string& path, uid_t uid) {
  struct stat s;
  if (stat(path.c_str(), &s) == 0) {
    return s.st_uid == uid;
  }
  return false;
}

inline std::string dirname(const std::string& path) {
  size_t pos = impl::get_dirname_position(path);
  if (pos == 0) {
    return ".";
  }
  return path.substr(0, pos);
}

inline std::optional<std::string> realpath(const std::string& path) {
  std::array<char, PATH_MAX> resolved_path;
  if (!::realpath(path.c_str(), &(resolved_path[0]))) {
    return std::nullopt;
  }
  return std::string(&(resolved_path[0]));
}

inline std::optional<mode_t> file_access_permissions(const std::string& path) {
  struct stat s;
  if (stat(path.c_str(), &s) != 0) {
    return std::nullopt;
  }
  return s.st_mode & ACCESSPERMS;
}

inline std::optional<off_t> file_size(const std::string& path) {
  struct stat s;
  if (stat(path.c_str(), &s) != 0) {
    return std::nullopt;
  }
  return s.st_size;
}

inline bool create_directory_with_intermediate_directories(const std::string& path, mode_t mode) {
  std::vector<std::string> parents;
  auto directory = path;
  while (!exists(directory)) {
    parents.push_back(directory);
    directory = dirname(directory);
  }

  for (auto it = std::rbegin(parents); it != std::rend(parents); std::advance(it, 1)) {
    if (mkdir(it->c_str(), mode) != 0) {
      return false;
    }
  }

  if (!is_directory(path)) {
    return false;
  }

  chmod(path.c_str(), mode);

  return true;
}

inline void copy(const std::string& from_file_path,
                 const std::string& to_file_path) {
  std::ifstream ifstream(from_file_path);
  if (!ifstream) {
    return;
  }
  std::ofstream ofstream(to_file_path);
  if (!ofstream) {
    return;
  }
  ofstream << ifstream.rdbuf();
}
} // namespace filesystem
} // namespace pqrs
