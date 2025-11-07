#pragma once

#include <IOKit/hidsystem/IOHIDLib.h>
#include <iostream>

namespace krbn {
namespace grabber {
namespace main {
int agent(void) {
  IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);

  return 0;
}
} // namespace main
} // namespace grabber
} // namespace krbn
