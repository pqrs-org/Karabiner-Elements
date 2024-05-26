#pragma once

// pqrs::osx::launchctl v3.1

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::launchctl` can be used safely in a multi-threaded environment.

#include "launchctl/domain_target.hpp"
#include "launchctl/service_name.hpp"
#include "launchctl/service_path.hpp"
#include "launchctl/service_target.hpp"
#include <type_safe/flag_set.hpp>

namespace pqrs {
namespace osx {
namespace launchctl {
inline void bootstrap(const domain_target& domain_target,
                      const service_path& service_path) {
  auto command = (std::stringstream() << "/bin/launchctl bootstrap " << domain_target << " " << service_path).str();
  system(command.c_str());
}

inline void bootout(const domain_target& domain_target,
                    const service_path& service_path) {
  auto command = (std::stringstream() << "/bin/launchctl bootout " << domain_target << " " << service_path).str();
  system(command.c_str());
}

inline void bootout(const domain_target& domain_target,
                    const service_name& service_name) {
  auto service_target = make_service_target(domain_target, service_name);

  auto command = (std::stringstream() << "/bin/launchctl bootout " << service_target).str();
  system(command.c_str());
}

inline void enable(const domain_target& domain_target,
                   const service_name& service_name) {
  auto service_target = make_service_target(domain_target, service_name);

  auto command = (std::stringstream() << "/bin/launchctl enable " << service_target).str();
  system(command.c_str());
}

inline void disable(const domain_target& domain_target,
                    const service_name& service_name) {
  auto service_target = make_service_target(domain_target, service_name);

  auto command = (std::stringstream() << "/bin/launchctl disable " << service_target).str();
  system(command.c_str());
}

enum class kickstart_flags {
  kill,
  background,
  _flag_set_size,
};

inline void kickstart(const domain_target& domain_target,
                      const service_name& service_name,
                      type_safe::flag_set<kickstart_flags> flags = type_safe::flag_set<kickstart_flags>()) {
  auto service_target = make_service_target(domain_target, service_name);

  std::stringstream ss;
  ss << "/bin/launchctl kickstart ";
  if (flags & kickstart_flags::kill) {
    ss << " -k ";
  }
  ss << service_target;

  if (flags & kickstart_flags::background) {
    ss << " & ";
  }

  auto command = ss.str();
  system(command.c_str());
}
} // namespace launchctl
} // namespace osx
} // namespace pqrs
