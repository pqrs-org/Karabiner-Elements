#include "components_manager_killer.hpp"
#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "filesystem_utility.hpp"
#include "grabber/components_manager.hpp"
#include "iokit_hid_device_open_checker_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
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
  // (grabber is launched from LaunchDaemons (root) and LaunchAgents (user).)
  //

  bool root = (geteuid() == 0);

  //
  // Setup logger
  //

  if (root) {
    krbn::logger::set_async_rotating_logger("grabber",
                                            "/var/log/karabiner/grabber.log",
                                            0755);
  } else {
    krbn::logger::set_async_rotating_logger("grabber_agent",
                                            krbn::constants::get_user_log_directory() + "/grabber_agent.log",
                                            0700);
  }

  krbn::logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Check another process
  //

  {
    bool another_process_running = false;

    if (root) {
      std::string pid_file_path = krbn::constants::get_pid_directory() + "/karabiner_grabber.pid";
      another_process_running = !krbn::process_utility::lock_single_application(pid_file_path);
    } else {
      another_process_running = !krbn::process_utility::lock_single_application_with_user_pid_file("karabiner_grabber_agent.pid");
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
        krbn::constants::get_grabber_state_json_file_path());
  }

  //
  // Open IOHIDDevices in order to gain input monitoring permission.
  // (Both from LaunchDaemons and LaunchAgents)
  //

  if (!krbn::iokit_hid_device_open_checker_utility::run_checker()) {
    if (state_json_writer) {
      state_json_writer->set("hid_device_open_permitted", false);
    }

    if (root) {
      // We have to restart this process in order to reflect the input monitoring approval.
      return 0;
    }

  } else {
    if (state_json_writer) {
      state_json_writer->set("hid_device_open_permitted", true);
    }
  }

  //
  // Make socket directory.
  //

  if (root) {
    krbn::filesystem_utility::mkdir_tmp_directory();

    unlink(krbn::constants::get_grabber_socket_file_path());

    // Make console_user_server_socket_directory

    std::stringstream ss;
    ss << "rm -r '" << krbn::constants::get_console_user_server_socket_directory() << "'";
    system(ss.str().c_str());

    mkdir(krbn::constants::get_console_user_server_socket_directory(), 0755);
    chown(krbn::constants::get_console_user_server_socket_directory(), 0, 0);
    chmod(krbn::constants::get_console_user_server_socket_directory(), 0755);
  }

  //
  // Run components_manager
  //

  krbn::components_manager_killer::initialize_shared_components_manager_killer();

  // We have to use raw pointer (not smart pointer) to delete it in `dispatch_async`.
  krbn::grabber::components_manager* components_manager = nullptr;

  if (auto killer = krbn::components_manager_killer::get_shared_components_manager_killer()) {
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

  if (root) {
    components_manager = new krbn::grabber::components_manager();
    components_manager->async_start();
  }

  CFRunLoopRun();

  krbn::components_manager_killer::terminate_shared_components_manager_killer();

  state_json_writer = nullptr;

  krbn::logger::get_logger()->info("karabiner_grabber is terminated.");

  return 0;
}
