#pragma once

// pqrs::osx::launchctl v1.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::launchctl` can be used safely in a multi-threaded environment.

#include "launchctl/domain_target.hpp"
#include "launchctl/service_name.hpp"
#include "launchctl/service_path.hpp"
#include "launchctl/service_target.hpp"

namespace pqrs {
namespace osx {
namespace launchctl {
inline void enable(const domain_target& domain_target,
                   const service_name& service_name,
                   const service_path& service_path) {
  auto service_target = make_service_target(domain_target, service_name);

  // If service_path is already bootstrapped and disabled, launchctl bootstrap will fail until it is enabled again.
  // So we should enable it first, and then bootstrap and enable it.

  {
    auto command = (std::stringstream() << "/bin/launchctl enable " << service_target).str();
    system(command.c_str());
  }
  {
    auto command = (std::stringstream() << "/bin/launchctl bootstrap " << domain_target << " " << service_path).str();
    system(command.c_str());
  }
  {
    auto command = (std::stringstream() << "/bin/launchctl enable " << service_target).str();
    system(command.c_str());
  }
}

inline void disable(const domain_target& domain_target,
                    const service_name& service_name,
                    const service_path& service_path) {
  auto service_target = make_service_target(domain_target, service_name);

  {
    auto command = (std::stringstream() << "/bin/launchctl bootout " << domain_target << " " << service_path).str();
    system(command.c_str());
  }
  {
    auto command = (std::stringstream() << "/bin/launchctl disable " << service_target).str();
    system(command.c_str());
  }
}

inline void kickstart(const domain_target& domain_target,
                      const service_name& service_name,
                      bool kill = false) {
  auto service_target = make_service_target(domain_target, service_name);

  {
    std::stringstream ss;
    ss << "/bin/launchctl kickstart ";
    if (kill) {
      ss << " -k ";
    }
    ss << service_target;

    auto command = ss.str();
    system(command.c_str());
  }
}
} // namespace launchctl
} // namespace osx
} // namespace pqrs
