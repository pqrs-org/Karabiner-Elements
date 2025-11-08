#pragma once

#include "app_icon.hpp"
#include "codesign_manager.hpp"
#include "constants.hpp"
#include "core_service/components_manager.hpp"
#include "core_service/core_service_state_json_writer.hpp"
#include "core_service/core_service_utility.hpp"
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
namespace core_service {
namespace main {

int daemon(void) {
  // Note:
  // Processes running as root should not rely on environment variables,
  // so we do not load custom environment variables in the grabber daemon.

  //
  // Setup logger
  //

  logger::set_async_rotating_logger("grabber",
                                    "/var/log/karabiner/grabber.log",
                                    pqrs::spdlog::filesystem::log_directory_perms_0755);
  logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Check another process
  //

  {
    bool another_process_running = !process_utility::lock_single_application(constants::get_pid_directory() / "karabiner_grabber.pid");

    if (another_process_running) {
      auto message = "Exit since another process is running.";
      logger::get_logger()->info(message);
      std::cerr << message << std::endl;
      return 1;
    }
  }

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
  // Prepare state_json_writer
  //

  auto core_service_state_json_writer = std::make_shared<core_service::core_service_state_json_writer>();

  //
  // Update karabiner_grabber_state.json
  //

  if (IOHIDCheckAccess(kIOHIDRequestTypeListenEvent) == kIOHIDAccessTypeGranted) {
    core_service_state_json_writer->set_hid_device_open_permitted(true);
  } else {
    logger::get_logger()->warn("Input Monitoring is not granted");

    core_service_state_json_writer->set_hid_device_open_permitted(false);

    // IOHIDRequestAccess won't work correctly unless it's called from the agent.
    // Therefore, the daemon does not call IOHIDRequestAccess and
    // simply waits until Input Monitoring permission is granted.
    // The daemon exits once permission is granted, which triggers launchd to restart it.
    core_service_utility::wait_until_input_monitoring_granted();

    return 0;
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

  filesystem_utility::mkdir_tmp_directory();
  filesystem_utility::mkdir_rootonly_directory();

  //
  // Run components_manager
  //

  components_manager_killer::initialize_shared_components_manager_killer();

  // We have to use raw pointer (not smart pointer) to delete it in `dispatch_async`.
  core_service::components_manager* components_manager = nullptr;

  if (auto killer = components_manager_killer::get_shared_components_manager_killer()) {
    killer->kill_called.connect([&components_manager] {
      dispatch_async(dispatch_get_main_queue(), ^{
        {
          // Mark as main queue to avoid a deadlock in `pqrs::gcd::dispatch_sync_on_main_queue` in destructor.
          pqrs::gcd::scoped_running_on_main_queue_marker marker;

          if (components_manager) {
            delete components_manager;
          }
        }

        CFRunLoopStop(CFRunLoopGetCurrent());
      });
    });
  }

  components_manager = new core_service::components_manager(core_service_state_json_writer);
  components_manager->async_start();

  CFRunLoopRun();

  components_manager_killer::terminate_shared_components_manager_killer();

  core_service_state_json_writer = nullptr;

  logger::get_logger()->info("karabiner_grabber is terminated.");

  return 0;
}
} // namespace main
} // namespace core_service
} // namespace krbn
