#pragma once

#include <cstdint>
#include <iostream>
#include <type_safe/strong_typedef.hpp>
#include <unordered_set>

namespace krbn {
struct vendor_id : type_safe::strong_typedef<vendor_id, uint32_t>,
                   type_safe::strong_typedef_op::equality_comparison<vendor_id>,
                   type_safe::strong_typedef_op::relational_comparison<vendor_id> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const vendor_id& value) {
  return stream << type_safe::get(value);
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::vendor_id> : type_safe::hashable<krbn::vendor_id> {
};
} // namespace std
