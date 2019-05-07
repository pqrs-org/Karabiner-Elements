#include "components_manager.hpp"
#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "process_utility.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/filesystem.hpp>

int main(int argc, const char* argv[]) {
  // Setup logger

  if (!krbn::constants::get_user_log_directory().empty()) {
    krbn::logger::set_async_rotating_logger("session_monitor",
                                            krbn::constants::get_user_log_directory() + "/session_monitor.log",
                                            0700);
  }

  krbn::logger::get_logger()->info("version {0}", karabiner_version);

  // Check another process

  if (!krbn::process_utility::lock_single_application_with_user_pid_file("karabiner_session_monitor.pid")) {
    krbn::logger::get_logger()->info("Exit since another process is running.");
    exit(0);
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
