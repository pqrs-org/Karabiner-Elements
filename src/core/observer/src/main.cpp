#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "iokit_hid_device_open_checker_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "observer/components_manager.hpp"
#include "process_utility.hpp"
#include "state_json_writer.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  //
  // Initialize
  //

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  //
  // Check euid
  // (observer is launched from LaunchDaemons (root) and LaunchAgents (user).)
  //

  bool root = (geteuid() == 0);

  //
  // Setup logger
  //

  if (root) {
    krbn::logger::set_async_rotating_logger("observer",
                                            "/var/log/karabiner/observer.log",
                                            0755);
  } else {
    krbn::logger::set_async_rotating_logger("observer_agent",
                                            krbn::constants::get_user_log_directory() + "/observer_agent.log",
                                            0700);
  }

  krbn::logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Check another process
  //

  {
    bool another_process_running = false;

    if (root) {
      std::string pid_file_path = krbn::constants::get_pid_directory() + "/karabiner_observer.pid";
      another_process_running = !krbn::process_utility::lock_single_application(pid_file_path);
    } else {
      another_process_running = !krbn::process_utility::lock_single_application_with_user_pid_file("karabiner_observer_agent.pid");
    }

    if (another_process_running) {
      auto message = "Exit since another process is running.";
      krbn::logger::get_logger()->info(message);
      std::cerr << message << std::endl;
      return 1;
    }
  }

  //
  // Prepare state_json_writer
  //

  std::shared_ptr<krbn::state_json_writer> state_json_writer;
  if (root) {
    state_json_writer = std::make_shared<krbn::state_json_writer>(
        krbn::constants::get_observer_state_json_file_path());
  }

  //
  // Open IOHIDDevices in order to gain input monitoring permission.
  // (Both from LaunchDaemons and LaunchAgents)
  //

  if (!krbn::iokit_hid_device_open_checker_utility::run_checker()) {
    if (state_json_writer) {
      state_json_writer->set("hid_device_open_permitted", false);
    }
  } else {
    if (state_json_writer) {
      state_json_writer->set("hid_device_open_permitted", true);
    }
  }

  //
  // Run components_manager
  //

  // We have to use raw pointer (not smart pointer) to delete it in `dispatch_async`.
  krbn::observer::components_manager* components_manager = nullptr;

  auto version_monitor = std::make_shared<krbn::version_monitor>(krbn::constants::get_version_file_path());

  version_monitor->changed.connect([&components_manager](auto&& version) {
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

  if (root) {
    components_manager = new krbn::observer::components_manager(version_monitor);
  }

  version_monitor->async_start();
  if (components_manager) {
    components_manager->async_start();
  }

  CFRunLoopRun();

  version_monitor = nullptr;

  state_json_writer = nullptr;

  krbn::logger::get_logger()->info("karabiner_observer is terminated.");

  return 0;
}
