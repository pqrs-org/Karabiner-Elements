#pragma once

#include "app_icon.hpp"
#include "codesign_manager.hpp"
#include "constants.hpp"
#include "core_service/components_manager.hpp"
#include "core_service/grabber_state_json_writer.hpp"
#include "filesystem_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "process_utility.hpp"
#include "services_utility.hpp"
#include <IOKit/hidsystem/IOHIDLib.h>
#include <iostream>
#include <mach/mach.h>
#include <pqrs/osx/workspace.hpp>

namespace krbn {
namespace grabber {
namespace core_service_utility {

static constexpr const char* karabiner_core_service_path = "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Core-Service.app/Contents/MacOS/Karabiner-Core-Service";

inline void wait_until_input_monitoring_granted(void) {
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    auto exit_code = system(fmt::format("'{0}' input-monitoring-granted",
                                        karabiner_core_service_path)
                                .c_str());
    if (exit_code == 0) {
      break;
    }
  }
}

} // namespace core_service_utility
} // namespace grabber
} // namespace krbn
