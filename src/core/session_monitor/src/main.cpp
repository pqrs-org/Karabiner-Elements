#include "components_manager.hpp"
#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "process_utility.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  // - karabiner_session_monitor is invoked by user.
  // - karabiner_session_monitor is executed as root by setuid.
  if (geteuid() != 0) {
    std::cerr << "fatal: karabiner_session_monitor requires root privilege." << std::endl;
    exit(1);
  }

  // Setup logger

  krbn::logger::set_async_rotating_logger("session_monitor",
                                          fmt::format("/var/log/karabiner/session_monitor.{0}.log", getuid()),
                                          0755);

  krbn::logger::get_logger()->info("version {0}", karabiner_version);

  // Check another process

  {
    std::string pid_file_path = krbn::constants::get_pid_directory() +
                                fmt::format("/karabiner_session_monitor.{0}.pid", getuid());
    if (!krbn::process_utility::lock_single_application(pid_file_path)) {
      auto message = "Exit since another process is running.";
      krbn::logger::get_logger()->info(message);
      std::cerr << message << std::endl;
      exit(1);
    }
  }

  // Initialize

  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  // Run components_manager

  std::shared_ptr<krbn::components_manager> components_manager;

  auto version_monitor = std::make_shared<krbn::version_monitor>(krbn::constants::get_version_file_path());

  version_monitor->changed.connect([&](auto&& version) {
    dispatch_async(dispatch_get_main_queue(), ^{
      components_manager = nullptr;
      CFRunLoopStop(CFRunLoopGetCurrent());
    });
  });

  components_manager = std::make_shared<krbn::components_manager>(version_monitor);

  version_monitor->async_start();
  components_manager->async_start();

  CFRunLoopRun();

  version_monitor = nullptr;

  krbn::logger::get_logger()->info("karabiner_session_monitor is terminated.");

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
