#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/osx/input_source/properties.hpp>

namespace pqrs::osx::input_source {
[[nodiscard]] inline std::size_t hash_value(const properties& value) {
  return std::hash<properties>{}(value);
}
} // namespace pqrs::osx::input_source
