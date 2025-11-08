#pragma once

#include "grabber/core_service_utility.hpp"
#include <IOKit/hidsystem/IOHIDLib.h>
#include <iostream>

namespace krbn {
namespace grabber {
namespace main {
int agent(void) {
  IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);

  core_service_utility::wait_until_input_monitoring_granted();

  return 0;
}
} // namespace main
} // namespace grabber
} // namespace krbn
