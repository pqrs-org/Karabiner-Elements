#pragma once

#include <cstdint>
#include <iostream>
#include <pqrs/osx/iokit_types.hpp>
#include <type_safe/strong_typedef.hpp>
#include <unordered_set>

namespace krbn {
struct product_id : type_safe::strong_typedef<product_id, uint32_t>,
                    type_safe::strong_typedef_op::equality_comparison<product_id>,
                    type_safe::strong_typedef_op::relational_comparison<product_id> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const product_id& value) {
  return stream << type_safe::get(value);
}

inline product_id make_product_id(const pqrs::osx::iokit_hid_product_id& value) {
  return product_id(type_safe::get(value));
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::product_id> : type_safe::hashable<krbn::product_id> {
};
} // namespace std
