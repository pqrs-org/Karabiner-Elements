#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <type_safe/strong_typedef.hpp>
#include <unistd.h>

namespace pqrs {
namespace osx {
namespace launchctl {
struct domain_target : type_safe::strong_typedef<domain_target, std::string>,
                       type_safe::strong_typedef_op::equality_comparison<domain_target> {
  using strong_typedef::strong_typedef;
};

std::ostream& operator<<(std::ostream& os, const domain_target& value) {
  return os << type_safe::get(value);
}

inline domain_target make_system_domain_target(void) {
  return domain_target("system/");
}

inline domain_target make_gui_domain_target(uid_t uid) {
  return domain_target((std::stringstream() << "gui/" << uid << "/").str());
}
} // namespace launchctl
} // namespace osx
} // namespace pqrs
