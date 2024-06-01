#pragma once

// (C) Copyright Takayama Fumihiko 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <nlohmann/json.hpp>
#include <type_safe/strong_typedef.hpp>

namespace krbn {
struct karabiner_machine_identifier : type_safe::strong_typedef<karabiner_machine_identifier, std::string>,
                                      type_safe::strong_typedef_op::equality_comparison<karabiner_machine_identifier> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const karabiner_machine_identifier& value) {
  return stream << type_safe::get(value);
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::karabiner_machine_identifier> : type_safe::hashable<krbn::karabiner_machine_identifier> {
};
} // namespace std
