#pragma once

#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "grabber/components_manager.hpp"
#include "grabber/grabber_state_json_writer.hpp"
#include "iokit_hid_device_open_checker_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "process_utility.hpp"
#include <iostream>
#include <mach/mach.h>

namespace krbn {
namespace grabber {
namespace main {
int daemon(void) {
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
    bool another_process_running = !process_utility::lock_single_application(constants::get_pid_directory() + "/karabiner_grabber.pid");

    if (another_process_running) {
      auto message = "Exit since another process is running.";
      logger::get_logger()->info(message);
      std::cerr << message << std::endl;
      return 1;
    }
  }

  //
  // Prepare state_json_writer
  //

  auto grabber_state_json_writer = std::make_shared<grabber::grabber_state_json_writer>();

  //
  // Update karabiner_grabber_state.json
  //

  if (!iokit_hid_device_open_checker_utility::run_checker()) {
    grabber_state_json_writer->set_hid_device_open_permitted(false);

    // We have to restart this process in order to reflect the input monitoring approval.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return 0;

  } else {
    grabber_state_json_writer->set_hid_device_open_permitted(true);
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
  // Make socket directory.
  //

  {
    filesystem_utility::mkdir_tmp_directory();

    unlink(constants::get_grabber_socket_file_path());

    // Make console_user_server_socket_directory

    std::stringstream ss;
    ss << "rm -r '" << constants::get_console_user_server_socket_directory() << "'";
    system(ss.str().c_str());

    mkdir(constants::get_console_user_server_socket_directory(), 0755);
    chown(constants::get_console_user_server_socket_directory(), 0, 0);
    chmod(constants::get_console_user_server_socket_directory(), 0755);
  }

  //
  // Run components_manager
  //

  components_manager_killer::initialize_shared_components_manager_killer();

  // We have to use raw pointer (not smart pointer) to delete it in `dispatch_async`.
  grabber::components_manager* components_manager = nullptr;

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

  components_manager = new grabber::components_manager(grabber_state_json_writer);
  components_manager->async_start();

  CFRunLoopRun();

  components_manager_killer::terminate_shared_components_manager_killer();

  grabber_state_json_writer = nullptr;

  logger::get_logger()->info("karabiner_grabber is terminated.");

  return 0;
}
} // namespace main
} // namespace grabber
} // namespace krbn
