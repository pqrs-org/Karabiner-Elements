#pragma once

#include "core_service/agent/components_manager.hpp"
#include "core_service/core_service_utility.hpp"
#include "environment_variable_utility.hpp"
#include "logger.hpp"
#include <IOKit/hidsystem/IOHIDLib.h>
#include <iostream>
#include <pqrs/osx/accessibility.hpp>
#include <pqrs/osx/application.hpp>

namespace krbn {
namespace core_service {
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
    krbn::logger::set_async_rotating_logger("core_service (agent)",
                                            krbn::constants::get_user_log_directory() / "core_service.log",
                                            pqrs::spdlog::filesystem::log_directory_perms_0700);
  }

  krbn::logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Log custom environment variables
  //

  krbn::environment_variable_utility::log(environment_variables);

  //
  // Get codesign
  //

  get_shared_codesign_manager()->log();

  //
  // The agent opens karabiner.json to trigger the disk-access permission prompt,
  // in case ~/.config/karabiner is a symlink and karabiner.json lives under Documents or similar.
  //

  std::ifstream input(krbn::constants::get_user_core_configuration_file_path());

  //
  // Check input monitoring permission
  //

  IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);

  core_service_utility::wait_until_input_monitoring_granted();

  //
  // Check accessibility permission
  //

  pqrs::osx::accessibility::is_process_trusted_with_prompt();

  core_service_utility::wait_until_accessibility_process_trusted();

  //
  // Run components_manager
  //

  krbn::components_manager_killer::initialize_shared_components_manager_killer();

  // We have to use raw pointer (not smart pointer) to delete it in `dispatch_async`.
  krbn::core_service::agent::components_manager* components_manager = nullptr;

  if (auto killer = krbn::components_manager_killer::get_shared_components_manager_killer()) {
    killer->kill_called.connect([] {
      dispatch_async(dispatch_get_main_queue(), ^{
        pqrs::osx::application::stop();
      });
    });
  }

  components_manager = new krbn::core_service::agent::components_manager();
  components_manager->async_start();

  pqrs::osx::application::finish_launching();
  pqrs::osx::application::run();

  {
    // Mark as main queue to avoid a deadlock in `pqrs::gcd::dispatch_sync_on_main_queue` in destructor.
    pqrs::gcd::scoped_running_on_main_queue_marker marker;

    delete components_manager;
    components_manager = nullptr;
  }

  krbn::components_manager_killer::terminate_shared_components_manager_killer();

  return 0;
}
} // namespace main
} // namespace core_service
} // namespace krbn
