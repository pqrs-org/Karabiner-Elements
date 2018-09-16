#pragma once

#include "stream_utility.hpp"
#include <cstdint>
#include <type_safe/strong_typedef.hpp>

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
