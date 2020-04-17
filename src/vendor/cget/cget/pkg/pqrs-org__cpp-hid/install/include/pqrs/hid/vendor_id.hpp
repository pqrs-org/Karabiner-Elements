#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <functional>
#include <iostream>
#include <type_safe/strong_typedef.hpp>

namespace pqrs {
namespace hid {
namespace vendor_id {
struct value_t : type_safe::strong_typedef<value_t, uint64_t>,
                 type_safe::strong_typedef_op::equality_comparison<value_t>,
                 type_safe::strong_typedef_op::relational_comparison<value_t> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const value_t& value) {
  return stream << type_safe::get(value);
}
} // namespace vendor_id
} // namespace hid
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::hid::vendor_id::value_t> : type_safe::hashable<pqrs::hid::vendor_id::value_t> {
};
} // namespace std
