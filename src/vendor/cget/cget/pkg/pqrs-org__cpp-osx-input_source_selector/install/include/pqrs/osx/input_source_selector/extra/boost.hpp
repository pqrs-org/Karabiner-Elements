#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/osx/input_source_selector/specifier.hpp>

namespace boost {
inline std::size_t hash_value(const pqrs::osx::input_source_selector::specifier& value) {
  return std::hash<pqrs::osx::input_source_selector::specifier>{}(value);
}
} // namespace boost
