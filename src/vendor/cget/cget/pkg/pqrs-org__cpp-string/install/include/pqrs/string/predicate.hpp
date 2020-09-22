#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <string_view>

namespace pqrs {
namespace string {
// Polyfill of std::string_view::starts_with (C++20)
inline bool starts_with(const std::string_view& s, const std::string_view& prefix) {
  return s.compare(0, prefix.size(), prefix) == 0;
}

// Polyfill of std::string_view::ends_with (C++20)
inline bool ends_with(const std::string_view& s, const std::string_view& suffix) {
  if (s.size() < suffix.size()) {
    return false;
  }

  return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}
} // namespace string
} // namespace pqrs
