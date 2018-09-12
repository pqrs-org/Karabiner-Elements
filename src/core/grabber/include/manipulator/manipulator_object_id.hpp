#pragma once

#include <cstdint>
#include <type_safe/strong_typedef.hpp>

namespace krbn {
namespace manipulator {
struct manipulator_object_id : type_safe::strong_typedef<manipulator_object_id, std::intptr_t>,
                               type_safe::strong_typedef_op::equality_comparison<manipulator_object_id>,
                               type_safe::strong_typedef_op::integer_arithmetic<manipulator_object_id> {
  using strong_typedef::strong_typedef;
};

inline manipulator_object_id make_manipulator_object_id(void* p) {
  return manipulator_object_id(reinterpret_cast<std::intptr_t>(p));
}
} // namespace manipulator
} // namespace krbn

namespace std {
template <>
struct hash<krbn::manipulator::manipulator_object_id> : type_safe::hashable<krbn::manipulator::manipulator_object_id> {
};
} // namespace std
