#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "trim.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <utf8cpp/utf8.h>

namespace pqrs::string {

[[nodiscard]] inline std::string truncate(std::string_view s,
                                          std::size_t length,
                                          std::string_view placeholder = "...") {
  // Replace invalid character to ensure no invalid characters until the end of string before create substring.
  auto valid_string = utf8::replace_invalid(s);

  if (valid_string.length() <= length ||
      length <= placeholder.length()) {
    return trim_invalid_right_copy(valid_string.substr(0, length));

  } else {
    auto result = trim_invalid_right_copy(valid_string.substr(0, length - placeholder.length()));
    result += placeholder;
    return result;
  }
}

} // namespace pqrs::string
