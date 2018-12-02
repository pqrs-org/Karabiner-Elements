#pragma once

#include <cstdint>
#include <iostream>
#include <pqrs/osx/iokit_types.hpp>
#include <type_safe/strong_typedef.hpp>
#include <unordered_set>

namespace krbn {
struct device_id : type_safe::strong_typedef<device_id, uint64_t>,
                   type_safe::strong_typedef_op::equality_comparison<device_id>,
                   type_safe::strong_typedef_op::relational_comparison<device_id> {
  using strong_typedef::strong_typedef;
};

inline size_t hash_value(const device_id& value) {
  return std::hash<uint64_t>{}(type_safe::get(value));
}

inline std::ostream& operator<<(std::ostream& stream, const device_id& value) {
  return stream << type_safe::get(value);
}

inline device_id make_device_id(const pqrs::osx::iokit_registry_entry_id& value) {
  return device_id(type_safe::get(value));
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::device_id> : type_safe::hashable<krbn::device_id> {
};
} // namespace std
