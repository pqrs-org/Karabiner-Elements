#pragma once

#include "types/core_service_permission_check_result.hpp"
#include <IOKit/hidsystem/IOHIDLib.h>
#include <pqrs/osx/accessibility.hpp>

namespace krbn::core_service::core_service_utility {

inline core_service_permission_check_result make_current_process_permission_check_result() {
  core_service_permission_check_result result;
  result.set_iohid_listen_event_allowed(IOHIDCheckAccess(kIOHIDRequestTypeListenEvent) == kIOHIDAccessTypeGranted);
  result.set_accessibility_process_trusted(pqrs::osx::accessibility::is_process_trusted());
  return result;
}

} // namespace krbn::core_service::core_service_utility
