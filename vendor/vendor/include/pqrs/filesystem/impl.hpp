#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <string>

namespace pqrs {
namespace filesystem {
namespace impl {
inline size_t get_dirname_position(const std::string& path, size_t pos = std::string::npos) {
  if (path.empty()) return 0;

  if (pos == std::string::npos) {
    pos = path.size() - 1;
  }

  if (path.size() <= pos) return 0;

  if (pos == 0) {
    // We retain the first slash for dirname("/") == "/".
    if (path[pos] == '/') {
      return 1;
    } else {
      return 0;
    }
  }

  if (path[pos] == '/') {
    --pos;
  }

  size_t i = path.rfind('/', pos);
  if (i == std::string::npos) {
    return 0;
  }
  if (i == 0) {
    // path starts with "/".
    return 1;
  }
  return i;
}
} // namespace impl
} // namespace filesystem
} // namespace pqrs
