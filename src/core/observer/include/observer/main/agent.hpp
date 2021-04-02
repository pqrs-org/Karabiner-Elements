#pragma once

#include "constants.hpp"
#include "iokit_hid_device_open_checker_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "process_utility.hpp"
#include <iostream>

namespace krbn {
namespace observer {
namespace main {
int agent(void) {
  //
  // Setup logger
  //

  logger::set_async_rotating_logger("observer_agent",
                                    constants::get_user_log_directory() / "observer_agent.log",
                                    pqrs::spdlog::filesystem::log_directory_perms_0700);
  logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Check another process
  //

  {
    bool another_process_running = !process_utility::lock_single_application_with_user_pid_file("karabiner_observer_agent.pid");

    if (another_process_running) {
      auto message = "Exit since another process is running.";
      logger::get_logger()->info(message);
      std::cerr << message << std::endl;
      return 1;
    }
  }

  //
  // Open IOHIDDevices in order to gain input monitoring permission.
  // (It requires to be run from from LaunchAgents.)
  //

  iokit_hid_device_open_checker_utility::run_checker();

  return 0;
}
} // namespace main
} // namespace observer
} // namespace krbn
