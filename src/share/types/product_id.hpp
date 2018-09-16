#pragma once

#include "stream_utility.hpp"
#include <cstdint>
#include <type_safe/strong_typedef.hpp>

namespace krbn {
struct product_id : type_safe::strong_typedef<product_id, uint32_t>,
                    type_safe::strong_typedef_op::equality_comparison<product_id>,
                    type_safe::strong_typedef_op::relational_comparison<product_id> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const product_id& value) {
  return stream << type_safe::get(value);
}
} // namespace krbn
