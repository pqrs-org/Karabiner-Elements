#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "karabiner_version.h"
#include "kextd/components_manager.hpp"
#include "logger.hpp"
#include "monitor/version_monitor.hpp"
#include "process_utility.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  if (geteuid() != 0) {
    std::cerr << "fatal: karabiner_kextd requires root privilege." << std::endl;
    exit(1);
  }

  //
  // Setup logger
  //

  krbn::logger::set_async_rotating_logger("kextd",
                                          "/var/log/karabiner/kextd.log",
                                          0755);

  krbn::logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Check another process
  //

  {
    std::string pid_file_path = krbn::constants::get_pid_directory() + "/karabiner_kextd.pid";
    if (!krbn::process_utility::lock_single_application(pid_file_path)) {
      std::string message("Exit since another process is running.");
      krbn::logger::get_logger()->info(message);
      std::cerr << message << std::endl;
      return 0;
    }
  }

  //
  // Initialize
  //

  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  //
  // Run components_manager
  //

  std::shared_ptr<krbn::kextd::components_manager> components_manager;

  auto version_monitor = std::make_shared<krbn::version_monitor>(krbn::constants::get_version_file_path());

  version_monitor->changed.connect([&](auto&& version) {
    dispatch_async(dispatch_get_main_queue(), ^{
      components_manager = nullptr;
      CFRunLoopStop(CFRunLoopGetCurrent());
    });
  });

  components_manager = std::make_shared<krbn::kextd::components_manager>();

  version_monitor->async_start();
  components_manager->async_start();

  CFRunLoopRun();

  version_monitor = nullptr;

  krbn::logger::get_logger()->info("karabiner_kextd is terminated.");

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
