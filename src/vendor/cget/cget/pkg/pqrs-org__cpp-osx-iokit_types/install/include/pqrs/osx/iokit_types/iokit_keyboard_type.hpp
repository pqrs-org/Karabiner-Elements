#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <compare>
#include <functional>
#include <iostream>
#include <type_safe/strong_typedef.hpp>

namespace pqrs {
namespace osx {
namespace iokit_keyboard_type {
struct value_t : type_safe::strong_typedef<value_t, uint64_t>,
                 type_safe::strong_typedef_op::equality_comparison<value_t>,
                 type_safe::strong_typedef_op::relational_comparison<value_t> {
  using strong_typedef::strong_typedef;

  constexpr auto operator<=>(const value_t& other) const {
    return type_safe::get(*this) <=> type_safe::get(other);
  }
};

inline std::ostream& operator<<(std::ostream& stream, const value_t& value) {
  return stream << type_safe::get(value);
}

//
// values
//

constexpr value_t ansi(40);
constexpr value_t iso(41);
constexpr value_t jis(42);

//
// methods
//

inline std::string make_string(value_t t) {
  if (t == iso) {
    return "iso";
  } else if (t == jis) {
    return "jis";
  }
  return "ansi";
}
} // namespace iokit_keyboard_type
} // namespace osx
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::osx::iokit_keyboard_type::value_t> : type_safe::hashable<pqrs::osx::iokit_keyboard_type::value_t> {
};
} // namespace std
