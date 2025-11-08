#pragma once

#include "core_service/core_service_utility.hpp"
#include <IOKit/hidsystem/IOHIDLib.h>
#include <iostream>

namespace krbn {
namespace core_service {
namespace main {
int agent(void) {
  IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);

  core_service_utility::wait_until_input_monitoring_granted();

  return 0;
}
} // namespace main
} // namespace core_service
} // namespace krbn
