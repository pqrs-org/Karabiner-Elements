#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <string_view>

namespace pqrs {
namespace string {
inline std::string truncate(const std::string_view& s,
                            size_t length,
                            const std::string_view& placeholder = "...") {
  if (s.length() <= length) {
    return std::string(s);
  }

  if (length <= placeholder.length()) {
    return std::string(s.substr(0, length));
  }

  std::stringstream ss;
  ss << s.substr(0, length - placeholder.length())
     << placeholder;
  return ss.str();
}
} // namespace string
} // namespace pqrs
