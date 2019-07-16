#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "iokit_hid_device_open_checker_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "observer/components_manager.hpp"
#include "process_utility.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  //
  // Initialize
  //

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  //
  // Open IOHIDDevices in order to gain input monitoring permission.
  // (Both from LaunchDaemons and LaunchAgents)
  //

  if (!krbn::iokit_hid_device_open_checker_utility::run_checker()) {
    return 0;
  }

  //
  // Check euid
  //

  if (geteuid() != 0) {
    return 0;
  }

  // Setup logger

  krbn::logger::set_async_rotating_logger("observer",
                                          "/var/log/karabiner/observer.log",
                                          0755);

  krbn::logger::get_logger()->info("version {0}", karabiner_version);

  // Check another process

  {
    std::string pid_file_path = krbn::constants::get_pid_directory() + "/karabiner_observer.pid";
    if (!krbn::process_utility::lock_single_application(pid_file_path)) {
      auto message = "Exit since another process is running.";
      krbn::logger::get_logger()->info(message);
      std::cerr << message << std::endl;
      exit(1);
    }
  }

  // Run components_manager

  std::shared_ptr<krbn::observer::components_manager> components_manager;

  auto version_monitor = std::make_shared<krbn::version_monitor>(krbn::constants::get_version_file_path());

  version_monitor->changed.connect([&](auto&& version) {
    dispatch_async(dispatch_get_main_queue(), ^{
      components_manager = nullptr;
      CFRunLoopStop(CFRunLoopGetCurrent());
    });
  });

  components_manager = std::make_shared<krbn::observer::components_manager>(version_monitor);

  version_monitor->async_start();
  components_manager->async_start();

  CFRunLoopRun();

  version_monitor = nullptr;

  krbn::logger::get_logger()->info("karabiner_observer is terminated.");

  return 0;
}
