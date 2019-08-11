#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "domain_target.hpp"
#include "service_name.hpp"

namespace pqrs {
namespace osx {
namespace launchctl {
struct service_target : type_safe::strong_typedef<service_target, std::string>,
                        type_safe::strong_typedef_op::equality_comparison<service_target> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& os, const service_target& value) {
  return os << type_safe::get(value);
}

inline service_target make_service_target(const domain_target& domain_target,
                                          const service_name& service_name) {
  return service_target(type_safe::get(domain_target) + type_safe::get(service_name));
}
} // namespace launchctl
} // namespace osx
} // namespace pqrs
