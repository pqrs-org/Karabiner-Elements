#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <functional>
#include <iostream>
#include <type_safe/strong_typedef.hpp>

namespace pqrs {
namespace osx {
struct iokit_keyboard_type : type_safe::strong_typedef<iokit_keyboard_type, uint64_t>,
                             type_safe::strong_typedef_op::equality_comparison<iokit_keyboard_type>,
                             type_safe::strong_typedef_op::relational_comparison<iokit_keyboard_type> {
  using strong_typedef::strong_typedef;
};

inline std::string make_iokit_keyboard_type_string(iokit_keyboard_type t) {
  if (t == iokit_keyboard_type(41)) {
    return "iso";
  } else if (t == iokit_keyboard_type(42)) {
    return "jis";
  }
  return "ansi";
}

inline std::ostream& operator<<(std::ostream& stream, const iokit_keyboard_type& value) {
  return stream << type_safe::get(value);
}
} // namespace osx
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::osx::iokit_keyboard_type> : type_safe::hashable<pqrs::osx::iokit_keyboard_type> {
};
} // namespace std
