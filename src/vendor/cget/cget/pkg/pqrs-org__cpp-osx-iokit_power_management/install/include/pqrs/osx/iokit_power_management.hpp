#pragma once

// pqrs::osx::iokit_power_management v1.0

// (C) Copyright Takayama Fumihiko 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::iokit_power_management` can be used safely in a multi-threaded environment.

#include "iokit_power_management/monitor.hpp"

namespace pqrs {
namespace osx {
namespace iokit_power_management {

inline iokit_return sleep(void) {
  auto port = IOPMFindPowerManagement(MACH_PORT_NULL);
  if (port != IO_OBJECT_NULL) {
    iokit_return r = IOPMSleepSystem(port);
    IOServiceClose(port);

    return r;
  }

  return kIOReturnError;
}

} // namespace iokit_power_management
} // namespace osx
} // namespace pqrs
