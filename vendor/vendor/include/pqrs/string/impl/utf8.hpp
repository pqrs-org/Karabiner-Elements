#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace pqrs::string::impl {

struct utf8_sequence {
  std::size_t length;
  bool valid;
};

struct cesu8_sequence {
  std::uint32_t code_point;
  std::size_t length;
  bool valid;
};

[[nodiscard]] constexpr unsigned char utf8_byte(std::string_view s,
                                                std::size_t position) noexcept {
  return static_cast<unsigned char>(s[position]);
}

[[nodiscard]] constexpr bool is_utf8_trail(unsigned char byte) noexcept {
  return (byte & 0xc0) == 0x80;
}

inline void append_utf8_code_point(std::string& output,
                                   std::uint32_t code_point) {
  if (code_point <= 0x7f) {
    output.push_back(static_cast<char>(code_point));
  } else if (code_point <= 0x7ff) {
    output.push_back(static_cast<char>(0xc0 | (code_point >> 6)));
    output.push_back(static_cast<char>(0x80 | (code_point & 0x3f)));
  } else if (code_point <= 0xffff) {
    output.push_back(static_cast<char>(0xe0 | (code_point >> 12)));
    output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3f)));
    output.push_back(static_cast<char>(0x80 | (code_point & 0x3f)));
  } else {
    output.push_back(static_cast<char>(0xf0 | (code_point >> 18)));
    output.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3f)));
    output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3f)));
    output.push_back(static_cast<char>(0x80 | (code_point & 0x3f)));
  }
}

[[nodiscard]] constexpr cesu8_sequence inspect_cesu8_sequence(
    std::string_view s,
    std::size_t position) noexcept {
  if (position >= s.size()) {
    return {0, 0, false};
  }

  const auto lead = utf8_byte(s, position);

  if (lead <= 0x7f) {
    return {lead, 1, true};
  }

  std::size_t expected_length = 0;
  unsigned char trail1_lower_bound = 0x80;
  unsigned char trail1_upper_bound = 0xbf;

  if (lead >= 0xc2 && lead <= 0xdf) {
    expected_length = 2;
  } else if (lead == 0xe0) {
    expected_length = 3;
    trail1_lower_bound = 0xa0;
  } else if (lead >= 0xe1 && lead <= 0xef) {
    // Unlike UTF-8, CESU-8 allows UTF-16 surrogates encoded by ED A0..BF.
    expected_length = 3;
  } else if (lead == 0xf0) {
    expected_length = 4;
    trail1_lower_bound = 0x90;
  } else if (lead >= 0xf1 && lead <= 0xf3) {
    expected_length = 4;
  } else if (lead == 0xf4) {
    expected_length = 4;
    trail1_upper_bound = 0x8f;
  } else {
    return {0, 1, false};
  }

  if (position + 1 >= s.size()) {
    return {0, 1, false};
  }

  const auto trail1 = utf8_byte(s, position + 1);
  if (trail1 < trail1_lower_bound || trail1 > trail1_upper_bound) {
    return {0, 1, false};
  }

  for (std::size_t length = 2; length < expected_length; ++length) {
    if (position + length >= s.size() ||
        !is_utf8_trail(utf8_byte(s, position + length))) {
      return {0, length, false};
    }
  }

  std::uint32_t code_point = 0;
  switch (expected_length) {
    case 2:
      code_point = static_cast<std::uint32_t>(((lead & 0x1f) << 6) |
                                              (trail1 & 0x3f));
      break;

    case 3:
      code_point = static_cast<std::uint32_t>(
          ((lead & 0x0f) << 12) |
          ((trail1 & 0x3f) << 6) |
          (utf8_byte(s, position + 2) & 0x3f));
      break;

    case 4:
      code_point = static_cast<std::uint32_t>(
          ((lead & 0x07) << 18) |
          ((trail1 & 0x3f) << 12) |
          ((utf8_byte(s, position + 2) & 0x3f) << 6) |
          (utf8_byte(s, position + 3) & 0x3f));
      break;
  }

  return {code_point, expected_length, true};
}

[[nodiscard]] constexpr utf8_sequence inspect_utf8_sequence(
    std::string_view s,
    std::size_t position) noexcept {
  if (position >= s.size()) {
    return {0, false};
  }

  const auto lead = utf8_byte(s, position);

  if (lead < 0x80) {
    return {1, true};
  }

  // The length of an invalid sequence is its maximal subpart: the longest
  // prefix that could still have become a well-formed UTF-8 sequence.
  std::size_t expected_length = 0;
  unsigned char trail1_lower_bound = 0x80;
  unsigned char trail1_upper_bound = 0xbf;

  if (lead >= 0xc2 && lead <= 0xdf) {
    expected_length = 2;
  } else if (lead == 0xe0) {
    expected_length = 3;
    trail1_lower_bound = 0xa0;
  } else if (lead >= 0xe1 && lead <= 0xec) {
    expected_length = 3;
  } else if (lead == 0xed) {
    expected_length = 3;
    trail1_upper_bound = 0x9f;
  } else if (lead >= 0xee && lead <= 0xef) {
    expected_length = 3;
  } else if (lead == 0xf0) {
    expected_length = 4;
    trail1_lower_bound = 0x90;
  } else if (lead >= 0xf1 && lead <= 0xf3) {
    expected_length = 4;
  } else if (lead == 0xf4) {
    expected_length = 4;
    trail1_upper_bound = 0x8f;
  } else {
    return {1, false};
  }

  if (position + 1 >= s.size()) {
    return {1, false};
  }

  const auto trail1 = utf8_byte(s, position + 1);
  if (trail1 < trail1_lower_bound || trail1 > trail1_upper_bound) {
    return {1, false};
  }

  for (std::size_t length = 2; length < expected_length; ++length) {
    if (position + length >= s.size() ||
        !is_utf8_trail(utf8_byte(s, position + length))) {
      return {length, false};
    }
  }

  return {expected_length, true};
}

[[nodiscard]] constexpr std::size_t find_invalid_utf8(std::string_view s) noexcept {
  std::size_t position = 0;
  while (position < s.size()) {
    const auto sequence = inspect_utf8_sequence(s, position);
    if (!sequence.valid) {
      return position;
    }
    position += sequence.length;
  }

  return std::string_view::npos;
}

} // namespace pqrs::string::impl
