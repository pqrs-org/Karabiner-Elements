#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <type_safe/strong_typedef.hpp>
#include <unistd.h>

namespace pqrs::osx::launchctl {
struct domain_target : type_safe::strong_typedef<domain_target, std::string>,
                       type_safe::strong_typedef_op::equality_comparison<domain_target> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& os, const domain_target& value) {
  return os << type_safe::get(value);
}

[[nodiscard]] inline domain_target make_system_domain_target() {
  return domain_target("system/");
}

[[nodiscard]] inline domain_target make_gui_domain_target(uid_t uid = getuid()) {
  return domain_target((std::stringstream() << "gui/" << uid << "/").str());
}
} // namespace pqrs::osx::launchctl
