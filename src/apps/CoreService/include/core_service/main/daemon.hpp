#pragma once

#include "app_icon.hpp"
#include "codesign_manager.hpp"
#include "constants.hpp"
#include "core_service/core_service_utility.hpp"
#include "core_service/daemon/components_manager.hpp"
#include "core_service/daemon/core_service_daemon_state_manager.hpp"
#include "filesystem_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "process_lifecycle_manager.hpp"
#include "services_utility.hpp"
#include "termination_signal_monitor.hpp"
#include <iostream>
#include <mach/mach.h>
#include <pqrs/osx/workspace.hpp>

namespace krbn::core_service::main {

int daemon() {
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

  system("/bin/bash '/Library/Application Support/org.pqrs/Karabiner-Elements/repair.sh'");

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
    auto permission_check_result = core_service_utility::make_current_process_permission_check_result();
    core_service_daemon_state_manager->set_current_process_permission_check_result(permission_check_result);
    // Immediately after startup, current_process_permission_check_result and bundle_permission_check_result are the same, so set both.
    core_service_daemon_state_manager->set_bundle_permission_check_result(permission_check_result);
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
      logger::get_logger()->debug("task_policy_set is called.");
    } else {
      logger::get_logger()->warn("task_policy_set error: {0}", kr);
    }
  }

  //
  // Make directories.
  //

  filesystem_utility::prepare_system_directories(std::nullopt);

  //
  // Run process_lifecycle_manager
  //

  auto weak_core_service_daemon_state_manager = std::weak_ptr<daemon::core_service_daemon_state_manager>(core_service_daemon_state_manager);
  process_lifecycle_manager::initialize_shared_instance(
      process_lifecycle_manager::configuration{
          .components_manager_maker =
              [weak_core_service_daemon_state_manager] {
                return std::make_unique<daemon::components_manager>(weak_core_service_daemon_state_manager);
              },
          .termination_completion_handler =
              [] {
                dispatch_async(dispatch_get_main_queue(), ^{
                  CFRunLoopStop(CFRunLoopGetCurrent());
                });
              },
          // Give device_grabber time to release seized devices asynchronously
          // before acknowledging the system sleep notification.
          .system_will_sleep_delay = std::chrono::seconds(3),
      });

  termination_signal_monitor signal_monitor([](int) {
    process_lifecycle_manager::async_request_termination();
  });

  process_lifecycle_manager::async_start();

  CFRunLoopRun();

  process_lifecycle_manager::terminate_shared_instance();

  //
  // Cleanup
  //

  core_service_daemon_state_manager = nullptr;

  logger::get_logger()->info("Karabiner-Core-Service is terminated.");

  return 0;
}
} // namespace krbn::core_service::main
