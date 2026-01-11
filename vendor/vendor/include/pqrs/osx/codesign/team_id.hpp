#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <type_safe/strong_typedef.hpp>

namespace pqrs {
namespace osx {
namespace codesign {

struct team_id : type_safe::strong_typedef<team_id, std::string>,
                 type_safe::strong_typedef_op::equality_comparison<team_id> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const team_id& value) {
  return stream << type_safe::get(value);
}

} // namespace codesign
} // namespace osx
} // namespace pqrs
