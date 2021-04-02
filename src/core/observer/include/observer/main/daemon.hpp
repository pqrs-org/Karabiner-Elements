#pragma once

#include "constants.hpp"
#include "iokit_hid_device_open_checker_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "observer/components_manager.hpp"
#include "observer/observer_state_json_writer.hpp"
#include "process_utility.hpp"
#include <iostream>

namespace krbn {
namespace observer {
namespace main {
int daemon(void) {
  //
  // Setup logger
  //

  logger::set_async_rotating_logger("observer",
                                    "/var/log/karabiner/observer.log",
                                    pqrs::spdlog::filesystem::log_directory_perms_0755);
  logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Check another process
  //

  {
    bool another_process_running = !process_utility::lock_single_application(constants::get_pid_directory() / "karabiner_observer.pid");

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

  auto observer_state_json_writer = std::make_shared<observer::observer_state_json_writer>();

  //
  // Update karabiner_observer_state.json
  //

  if (!iokit_hid_device_open_checker_utility::run_checker()) {
    observer_state_json_writer->set_hid_device_open_permitted(false);

    // We have to restart this process in order to reflect the input monitoring approval.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return 0;

  } else {
    observer_state_json_writer->set_hid_device_open_permitted(true);
  }

  //
  // Run components_manager
  //

  components_manager_killer::initialize_shared_components_manager_killer();

  // We have to use raw pointer (not smart pointer) to delete it in `dispatch_async`.
  observer::components_manager* components_manager = nullptr;

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

  components_manager = new observer::components_manager();
  components_manager->async_start();

  CFRunLoopRun();

  components_manager_killer::terminate_shared_components_manager_killer();

  observer_state_json_writer = nullptr;

  logger::get_logger()->info("karabiner_observer is terminated.");

  return 0;
}
} // namespace main
} // namespace observer
} // namespace krbn
