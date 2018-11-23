#pragma once

#include <cstdint>
#include <iostream>
#include <pqrs/osx/iokit_types.hpp>
#include <type_safe/strong_typedef.hpp>
#include <unordered_set>

namespace krbn {
struct location_id : type_safe::strong_typedef<location_id, uint32_t>,
                     type_safe::strong_typedef_op::equality_comparison<location_id>,
                     type_safe::strong_typedef_op::relational_comparison<location_id> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const location_id& value) {
  return stream << type_safe::get(value);
}

inline location_id make_location_id(const pqrs::osx::iokit_hid_location_id& value) {
  return location_id(type_safe::get(value));
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::location_id> : type_safe::hashable<krbn::location_id> {
};
} // namespace std
