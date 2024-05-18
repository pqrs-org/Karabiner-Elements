#pragma once

// (C) Copyright Takayama Fumihiko 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <compare>
#include <functional>
#include <iostream>
#include <type_safe/strong_typedef.hpp>

namespace pqrs {
namespace hid {
namespace product_string {
struct value_t : type_safe::strong_typedef<value_t, std::string>,
                 type_safe::strong_typedef_op::equality_comparison<value_t> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const value_t& value) {
  return stream << type_safe::get(value);
}
} // namespace product_string
} // namespace hid
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::hid::product_string::value_t> : type_safe::hashable<pqrs::hid::product_string::value_t> {
};
} // namespace std
