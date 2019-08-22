#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <type_safe/strong_typedef.hpp>

namespace pqrs {
namespace local_datagram {
struct request_id : type_safe::strong_typedef<request_id, uint64_t>,
                    type_safe::strong_typedef_op::equality_comparison<request_id>,
                    type_safe::strong_typedef_op::relational_comparison<request_id>,
                    type_safe::strong_typedef_op::integer_arithmetic<request_id> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const request_id& value) {
  return stream << type_safe::get(value);
}
} // namespace local_datagram
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::local_datagram::request_id> : type_safe::hashable<pqrs::local_datagram::request_id> {
};
} // namespace std
