#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <string>

namespace pqrs {
namespace string {
inline std::string truncate(const std::string& s,
                            size_t length,
                            const std::string& placeholder = "...") {
  if (s.length() <= length) {
    return s;
  }

  if (length <= placeholder.length()) {
    return s.substr(0, length);
  }

  return s.substr(0, length - placeholder.length()) + placeholder;
}
} // namespace string
} // namespace pqrs
