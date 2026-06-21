#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/osx/input_source_selector/specifier.hpp>

namespace pqrs::osx::input_source_selector {
[[nodiscard]] inline std::size_t hash_value(const specifier& value) {
  return std::hash<specifier>{}(value);
}
} // namespace pqrs::osx::input_source_selector
