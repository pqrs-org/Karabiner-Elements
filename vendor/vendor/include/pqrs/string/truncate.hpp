#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <string_view>
#include <utf8cpp/utf8.h>

namespace pqrs {
namespace string {
inline std::string truncate(const std::string_view& s,
                            size_t length,
                            const std::string_view& placeholder = "...") {
  // Replace invalid character to ensure no invalid characters until the end of string before create substring.
  auto valid_string = utf8::replace_invalid(s);

  if (valid_string.length() <= length ||
      length <= placeholder.length()) {
    return trim_invalid_right_copy(valid_string.substr(0, length));

  } else {
    // Append placeholder
    std::stringstream ss;
    ss << trim_invalid_right_copy(valid_string.substr(0, length - placeholder.length()))
       << placeholder;
    return ss.str();
  }
}
} // namespace string
} // namespace pqrs
