#pragma once

#include <cstdint>
#include <ostream>
#include <type_safe/strong_typedef.hpp>

namespace krbn {
struct absolute_time_duration : type_safe::strong_typedef<absolute_time_duration, int64_t>,
                                type_safe::strong_typedef_op::equality_comparison<absolute_time_duration>,
                                type_safe::strong_typedef_op::relational_comparison<absolute_time_duration>,
                                type_safe::strong_typedef_op::unary_plus<absolute_time_duration>,
                                type_safe::strong_typedef_op::unary_minus<absolute_time_duration>,
                                type_safe::strong_typedef_op::addition<absolute_time_duration>,
                                type_safe::strong_typedef_op::subtraction<absolute_time_duration>,
                                type_safe::strong_typedef_op::increment<absolute_time_duration>,
                                type_safe::strong_typedef_op::decrement<absolute_time_duration> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const absolute_time_duration& value) {
  return stream << type_safe::get(value);
}
} // namespace krbn
