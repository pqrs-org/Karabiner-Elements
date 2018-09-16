#pragma once

#include "stream_utility.hpp"
#include <cstdint>

namespace krbn {
struct registry_entry_id : type_safe::strong_typedef<registry_entry_id, uint64_t>,
                           type_safe::strong_typedef_op::equality_comparison<registry_entry_id>,
                           type_safe::strong_typedef_op::relational_comparison<registry_entry_id> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const registry_entry_id& value) {
  return stream << type_safe::get(value);
}
} // namespace krbn
