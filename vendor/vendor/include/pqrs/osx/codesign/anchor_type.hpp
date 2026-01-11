#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <ostream>
#include <string_view>

namespace pqrs {
namespace osx {
namespace codesign {

enum class anchor_type {
  none,
  apple,
  apple_generic,
};

inline std::string_view to_string(anchor_type value) {
  switch (value) {
    case anchor_type::none:
      return "none";
    case anchor_type::apple:
      return "apple";
    case anchor_type::apple_generic:
      return "apple_generic";
  }
  return "unknown";
}

inline std::ostream& operator<<(std::ostream& stream, anchor_type value) {
  return stream << to_string(value);
}

} // namespace codesign
} // namespace osx
} // namespace pqrs
