#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "filesystem_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "run_loop_thread_utility.hpp"
#include "session_monitor/components_manager.hpp"
#include <iostream>
#include <pqrs/gcd.hpp>
#include <pqrs/osx/process_info.hpp>

int main(int argc, const char* argv[]) {
  //
  // Initialize
  //

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::exit);

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  umask(0022);

  pqrs::osx::process_info::enable_sudden_termination();

  // Note:
  // Processes running as root should not rely on environment variables,
  // so we do not load custom environment variables in the session_monitor.

  //
  // Check euid
  //

  // - karabiner_session_monitor is invoked by user.
  // - karabiner_session_monitor is executed as root by setuid.
  if (geteuid() != 0) {
    std::cerr << "fatal: karabiner_session_monitor requires root privilege." << std::endl;
    return 1;
  }

  //
  // Setup logger
  //

  // We have to use `getuid` (not `geteuid`) since `karabiner_session_monitor` is run as root by suid.
  // (We have to make a log file which includes the real user ID in the file path.)
  krbn::logger::set_async_rotating_logger("session_monitor",
                                          fmt::format("/var/log/karabiner/session_monitor.{0}.log", getuid()),
                                          pqrs::spdlog::filesystem::log_directory_perms_0755);

  krbn::logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Make socket directory.
  //

  krbn::filesystem_utility::create_base_directories(std::nullopt);

  //
  // Run components_manager
  //

  krbn::components_manager_killer::initialize_shared_components_manager_killer();

  // We have to use a raw pointer to control the destruction timing from `kill_called`.
  krbn::session_monitor::components_manager* components_manager = nullptr;

  if (auto killer = krbn::components_manager_killer::get_shared_components_manager_killer()) {
    killer->kill_called.connect([&components_manager] {
      // Destroy `components_manager` before stopping the main run loop so that
      // cleanup code that uses `gcd::dispatch_sync_on_main_queue` can still run
      // while the main thread is servicing the run loop.
      // `kill_called` is invoked on the shared dispatcher thread, so we destroy
      // `components_manager` there before calling `CFRunLoopStop`.
      if (components_manager) {
        delete components_manager;
        components_manager = nullptr;
      }

      dispatch_async(dispatch_get_main_queue(), ^{
        CFRunLoopStop(CFRunLoopGetCurrent());
      });
    });
  }

  components_manager = new krbn::session_monitor::components_manager();
  components_manager->async_start();
  CFRunLoopRun();

  //
  // Cleanup
  //

  krbn::components_manager_killer::terminate_shared_components_manager_killer();

  krbn::logger::get_logger()->info("karabiner_session_monitor is terminated.");

  return 0;
}
