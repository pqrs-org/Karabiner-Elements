#pragma once

#include <cstdint>
#include <ostream>
#include <type_safe/strong_typedef.hpp>

namespace krbn {
struct absolute_time : type_safe::strong_typedef<absolute_time, uint64_t>,
                       type_safe::strong_typedef_op::equality_comparison<absolute_time>,
                       type_safe::strong_typedef_op::relational_comparison<absolute_time>,
                       type_safe::strong_typedef_op::integer_arithmetic<absolute_time> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const absolute_time& value) {
  return stream << type_safe::get(value);
}
} // namespace krbn
