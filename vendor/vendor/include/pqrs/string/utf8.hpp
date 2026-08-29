#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "impl/utf8.hpp"

#include <string>
#include <string_view>

namespace pqrs::string {

[[nodiscard]] inline std::string cesu8_to_utf8(std::string_view s) {
  constexpr std::uint32_t replacement_character = 0xfffd;
  constexpr std::uint32_t lead_surrogate_min = 0xd800;
  constexpr std::uint32_t lead_surrogate_max = 0xdbff;
  constexpr std::uint32_t trail_surrogate_min = 0xdc00;
  constexpr std::uint32_t trail_surrogate_max = 0xdfff;

  std::string result;
  result.reserve(s.size());

  std::uint32_t pending_lead_surrogate = 0;
  auto flush_pending_lead_surrogate = [&] {
    if (pending_lead_surrogate != 0) {
      impl::append_utf8_code_point(result, replacement_character);
      pending_lead_surrogate = 0;
    }
  };

  std::size_t position = 0;
  while (position < s.size()) {
    const auto sequence = impl::inspect_cesu8_sequence(s, position);
    if (!sequence.valid) {
      flush_pending_lead_surrogate();
      impl::append_utf8_code_point(result, replacement_character);
      position += sequence.length;
      continue;
    }
    position += sequence.length;

    const auto code_point = sequence.code_point;
    if (code_point >= lead_surrogate_min &&
        code_point <= lead_surrogate_max) {
      flush_pending_lead_surrogate();
      pending_lead_surrogate = code_point;
    } else if (code_point >= trail_surrogate_min &&
               code_point <= trail_surrogate_max) {
      if (pending_lead_surrogate != 0) {
        impl::append_utf8_code_point(
            result,
            0x10000 + ((pending_lead_surrogate - lead_surrogate_min) << 10) +
                (code_point - trail_surrogate_min));
        pending_lead_surrogate = 0;
      } else {
        impl::append_utf8_code_point(result, replacement_character);
      }
    } else {
      flush_pending_lead_surrogate();
      impl::append_utf8_code_point(result, code_point);
    }
  }

  flush_pending_lead_surrogate();
  return result;
}

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
