#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/osx/input_source/properties.hpp>

namespace pqrs {
namespace osx {
namespace input_source {
inline std::size_t hash_value(const properties& value) {
  return std::hash<properties>{}(value);
}
} // namespace input_source
} // namespace osx
} // namespace pqrs
