#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "impl/utf8.hpp"

#include <string>
#include <string_view>

namespace pqrs::string {

[[nodiscard]] inline std::string replace_invalid_utf8(std::string_view s) {
  constexpr std::string_view replacement = "\xef\xbf\xbd";

  std::string result;
  result.reserve(s.size());

  std::size_t position = 0;
  std::size_t valid_sequence_start = 0;
  while (position < s.size()) {
    const auto sequence = impl::inspect_utf8_sequence(s, position);
    if (sequence.valid) {
      position += sequence.length;
      continue;
    }

    result.append(s.substr(valid_sequence_start,
                           position - valid_sequence_start));
    result.append(replacement);
    position += sequence.length;
    valid_sequence_start = position;
  }

  result.append(s.substr(valid_sequence_start));
  return result;
}

} // namespace pqrs::string
