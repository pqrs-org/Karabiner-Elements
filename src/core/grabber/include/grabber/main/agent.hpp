#pragma once

#include "constants.hpp"
#include "environment_variable_utility.hpp"
#include "iokit_hid_device_open_checker_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "process_utility.hpp"
#include <iostream>

namespace krbn {
namespace grabber {
namespace main {
int agent(void) {
  //
  // Load custom environment variables
  //

  auto environment_variables = krbn::environment_variable_utility::load_custom_environment_variables();

  //
  // Setup logger
  //

  if (!krbn::constants::get_user_log_directory().empty()) {
    logger::set_async_rotating_logger("grabber_agent",
                                      constants::get_user_log_directory() / "grabber_agent.log",
                                      pqrs::spdlog::filesystem::log_directory_perms_0700);
  }
  logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Log custom environment variables
  //

  krbn::environment_variable_utility::log(environment_variables);

  //
  // Check another process
  //

  {
    bool another_process_running = !process_utility::lock_single_application_with_user_pid_file("karabiner_grabber_agent.pid");

    if (another_process_running) {
      auto message = "Exit since another process is running.";
      logger::get_logger()->info(message);
      std::cerr << message << std::endl;
      return 1;
    }
  }

  //
  // Open IOHIDDevices in order to gain input monitoring permission.
  //
  // Note:
  // It needs to be run with user permissions to trigger the approval request for Input Monitoring built into macOS.
  // Additionally, if another process starts grabber as a child process, macOS will determine that the Input Monitoring permission request came from the parent process.
  // Therefore, grabber needs to be called via launchd.
  // In conclusion, grabber must be started through LaunchAgents.
  //

  iokit_hid_device_open_checker_utility::run_checker();

  return 0;
}
} // namespace main
} // namespace grabber
} // namespace krbn
