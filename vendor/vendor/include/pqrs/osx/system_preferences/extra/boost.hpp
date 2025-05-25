#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/osx/system_preferences/keyboard_type_key.hpp>
#include <pqrs/osx/system_preferences/properties.hpp>

namespace pqrs {
namespace osx {
namespace system_preferences {
inline std::size_t hash_value(const keyboard_type_key& value) {
  return std::hash<keyboard_type_key>{}(value);
}
inline std::size_t hash_value(const properties& value) {
  return std::hash<properties>{}(value);
}
} // namespace system_preferences
} // namespace osx
} // namespace pqrs
