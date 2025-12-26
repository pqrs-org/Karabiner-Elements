#pragma once

#include "core_service/core_service_utility.hpp"
#include <IOKit/hidsystem/IOHIDLib.h>
#include <iostream>

namespace krbn {
namespace core_service {
namespace main {
int agent(void) {
  // The agent opens karabiner.json to trigger the disk-access permission prompt,
  // in case ~/.config/karabiner is a symlink and karabiner.json lives under Documents or similar.
  std::ifstream input(krbn::constants::get_user_core_configuration_file_path());

  IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);

  core_service_utility::wait_until_input_monitoring_granted();

  return 0;
}
} // namespace main
} // namespace core_service
} // namespace krbn
