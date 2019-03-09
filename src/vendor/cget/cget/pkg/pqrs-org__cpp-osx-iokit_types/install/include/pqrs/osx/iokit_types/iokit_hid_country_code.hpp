#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <functional>
#include <iostream>
#include <type_safe/strong_typedef.hpp>

namespace pqrs {
namespace osx {
struct iokit_hid_country_code : type_safe::strong_typedef<iokit_hid_country_code, uint64_t>,
                                type_safe::strong_typedef_op::equality_comparison<iokit_hid_country_code>,
                                type_safe::strong_typedef_op::relational_comparison<iokit_hid_country_code> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const iokit_hid_country_code& value) {
  return stream << type_safe::get(value);
}
} // namespace osx
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::osx::iokit_hid_country_code> : type_safe::hashable<pqrs::osx::iokit_hid_country_code> {
};
} // namespace std
