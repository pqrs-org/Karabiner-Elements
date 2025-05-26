#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <type_safe/strong_typedef.hpp>

namespace pqrs {
namespace osx {
namespace launchctl {
struct service_name : type_safe::strong_typedef<service_name, std::string>,
                      type_safe::strong_typedef_op::equality_comparison<service_name> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& os, const service_name& value) {
  return os << type_safe::get(value);
}
} // namespace launchctl
} // namespace osx
} // namespace pqrs
