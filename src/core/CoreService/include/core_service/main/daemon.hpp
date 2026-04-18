#pragma once

#include "app_icon.hpp"
#include "codesign_manager.hpp"
#include "constants.hpp"
#include "core_service/daemon/components_manager.hpp"
#include "core_service/daemon/core_service_daemon_state_manager.hpp"
#include "core_service/core_service_utility.hpp"
#include "filesystem_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "services_utility.hpp"
#include <iostream>
#include <mach/mach.h>
#include <pqrs/osx/workspace.hpp>

namespace krbn {
namespace core_service {
namespace main {

int daemon(void) {
  // Note:
  // Processes running as root should not rely on environment variables,
  // so we do not load custom environment variables in the core_service daemon.

  //
  // Setup logger
  //

  logger::set_async_rotating_logger("core_service (daemon)",
                                    "/var/log/karabiner/core_service.log",
                                    pqrs::spdlog::filesystem::log_directory_perms_0755);
  logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Get codesign
  //

  get_shared_codesign_manager()->log();

  //
  // Run repair.sh
  //

  system("/bin/sh '/Library/Application Support/org.pqrs/Karabiner-Elements/repair.sh'");

  //
  // Check Karabiner-Elements.app exists
  //

  auto settings_application_url = pqrs::osx::workspace::find_application_url_by_bundle_identifier("org.pqrs.Karabiner-Elements.Settings");
  logger::get_logger()->info("Karabiner-Elements.app path: {0}", settings_application_url);

  //
  // Prepare core_service_daemon_state_manager
  //

  auto core_service_daemon_state_manager = std::make_shared<daemon::core_service_daemon_state_manager>();
  {
    auto permission_check_result = core_service_utility::make_permission_check_result_for_current_process();
    core_service_daemon_state_manager->set_current_process_permission_check_result(permission_check_result);
  }

  //
  // Set task_qos_policy
  //

  {
    task_qos_policy qosinfo;

    memset(&qosinfo, 0, sizeof(qosinfo));
    qosinfo.task_latency_qos_tier = LATENCY_QOS_TIER_0;
    qosinfo.task_throughput_qos_tier = THROUGHPUT_QOS_TIER_0;
    pqrs::osx::kern_return kr = task_policy_set(mach_task_self(),
                                                TASK_BASE_QOS_POLICY,
                                                reinterpret_cast<task_policy_t>(&qosinfo),
                                                TASK_QOS_POLICY_COUNT);
    if (kr) {
      logger::get_logger()->info("task_policy_set is called.");
    } else {
      logger::get_logger()->warn("task_policy_set error: {0}", kr);
    }
  }

  //
  // Make directories.
  //

  filesystem_utility::create_base_directories(std::nullopt);

  //
  // Run components_manager
  //

  components_manager_killer::initialize_shared_components_manager_killer();

  // We have to use raw pointer (not smart pointer) to delete it in `dispatch_async`.
  daemon::components_manager* components_manager = nullptr;

  if (auto killer = components_manager_killer::get_shared_components_manager_killer()) {
    killer->kill_called.connect([] {
      dispatch_async(dispatch_get_main_queue(), ^{
        CFRunLoopStop(CFRunLoopGetCurrent());
      });
    });
  }

  components_manager = new daemon::components_manager(core_service_daemon_state_manager);
  components_manager->async_start();

  CFRunLoopRun();

  {
    // Mark as main queue to avoid a deadlock in `pqrs::gcd::dispatch_sync_on_main_queue` in destructor.
    pqrs::gcd::scoped_running_on_main_queue_marker marker;

    delete components_manager;
    components_manager = nullptr;
  }

  components_manager_killer::terminate_shared_components_manager_killer();

  core_service_daemon_state_manager = nullptr;

  logger::get_logger()->info("Karabiner-Core-Service is terminated.");

  return 0;
}
} // namespace main
} // namespace core_service
} // namespace krbn
