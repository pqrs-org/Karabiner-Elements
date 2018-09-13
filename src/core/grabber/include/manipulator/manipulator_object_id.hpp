#pragma once

#include <cstdint>
#include <type_safe/strong_typedef.hpp>

namespace krbn {
namespace manipulator {
struct manipulator_object_id : type_safe::strong_typedef<manipulator_object_id, uint64_t>,
                               type_safe::strong_typedef_op::equality_comparison<manipulator_object_id>,
                               type_safe::strong_typedef_op::integer_arithmetic<manipulator_object_id> {
  using strong_typedef::strong_typedef;
};

inline manipulator_object_id make_new_manipulator_object_id(void) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> guard(mutex);

  static uint64_t id = 0;
  return manipulator_object_id(++id);
}
} // namespace manipulator
} // namespace krbn

namespace std {
template <>
struct hash<krbn::manipulator::manipulator_object_id> : type_safe::hashable<krbn::manipulator::manipulator_object_id> {
};
} // namespace std
