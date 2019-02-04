#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <string>

namespace pqrs {
namespace string {
inline bool starts_with(const std::string& s, const std::string& prefix) {
  return s.compare(0, prefix.size(), prefix) == 0;
}

inline bool ends_with(const std::string& s, const std::string& suffix) {
  if (s.size() < suffix.size()) {
    return false;
  }

  return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}
} // namespace string
} // namespace pqrs
