#pragma once

#include <compare>
#include <functional>
#include <iostream>
#include <pqrs/json.hpp>
#include <type_safe/strong_typedef.hpp>

namespace krbn {
namespace event_integer_value {
struct value_t : type_safe::strong_typedef<value_t, int64_t>,
                 type_safe::strong_typedef_op::equality_comparison<value_t>,
                 type_safe::strong_typedef_op::relational_comparison<value_t> {
  using strong_typedef::strong_typedef;

  constexpr auto operator<=>(const value_t& other) const {
    return type_safe::get(*this) <=> type_safe::get(other);
  }
};

inline std::ostream& operator<<(std::ostream& stream, const value_t& value) {
  return stream << type_safe::get(value);
}

inline void to_json(nlohmann::json& j, const value_t& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, value_t& value) {
  pqrs::json::requires_number(j, "json");

  value = value_t(j.get<type_safe::underlying_type<value_t>>());
}
} // namespace event_integer_value
} // namespace krbn

namespace std {
template <>
struct hash<krbn::event_integer_value::value_t> : type_safe::hashable<krbn::event_integer_value::value_t> {
};
} // namespace std
