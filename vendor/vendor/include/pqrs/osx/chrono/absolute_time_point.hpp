#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "absolute_time_duration.hpp"

namespace pqrs {
namespace osx {
namespace chrono {
struct absolute_time_point : type_safe::strong_typedef<absolute_time_point, uint64_t>,
                             type_safe::strong_typedef_op::equality_comparison<absolute_time_point>,
                             type_safe::strong_typedef_op::relational_comparison<absolute_time_point>,
                             type_safe::strong_typedef_op::increment<absolute_time_point>,
                             type_safe::strong_typedef_op::decrement<absolute_time_point> {
  using strong_typedef::strong_typedef;
};

inline absolute_time_duration operator-(const absolute_time_point& a,
                                        const absolute_time_point& b) {
  return absolute_time_duration(type_safe::get(a) - type_safe::get(b));
}

inline absolute_time_point operator+(const absolute_time_point& a,
                                     const absolute_time_duration& b) {
  return absolute_time_point(type_safe::get(a) + type_safe::get(b));
}

inline absolute_time_point operator-(const absolute_time_point& a,
                                     const absolute_time_duration& b) {
  return absolute_time_point(type_safe::get(a) - type_safe::get(b));
}

inline absolute_time_point& operator+=(absolute_time_point& a,
                                       const absolute_time_duration& b) {
  a = a + b;
  return a;
}

inline absolute_time_point& operator-=(absolute_time_point& a,
                                       const absolute_time_duration& b) {
  a = a - b;
  return a;
}

inline std::ostream& operator<<(std::ostream& stream, const absolute_time_point& value) {
  return stream << type_safe::get(value);
}
} // namespace chrono
} // namespace osx
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::osx::chrono::absolute_time_point> : type_safe::hashable<pqrs::osx::chrono::absolute_time_point> {
};
} // namespace std
