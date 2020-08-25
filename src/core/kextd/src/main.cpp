#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "karabiner_version.h"
#include "kextd/components_manager.hpp"
#include "logger.hpp"
#include "process_utility.hpp"
#include <iostream>
#include <pqrs/gcd.hpp>

int main(int argc, const char* argv[]) {
  //
  // Initialize
  //

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  //
  // Check euid
  //

  if (geteuid() != 0) {
    std::cerr << "fatal: karabiner_kextd requires root privilege." << std::endl;
    return 1;
  }

  //
  // Setup logger
  //

  auto log_directory_perms = std::filesystem::perms::owner_all |
                             std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
                             std::filesystem::perms::others_read | std::filesystem::perms::others_exec;
  krbn::logger::set_async_rotating_logger("kextd",
                                          "/var/log/karabiner/kextd.log",
                                          log_directory_perms);

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
  // Run components_manager
  //

  krbn::components_manager_killer::initialize_shared_components_manager_killer();

  // We have to use raw pointer (not smart pointer) to delete it in `dispatch_async`.
  krbn::kextd::components_manager* components_manager = nullptr;

  if (auto killer = krbn::components_manager_killer::get_shared_components_manager_killer()) {
    killer->kill_called.connect([&components_manager] {
      dispatch_async(dispatch_get_main_queue(), ^{
        {
          // Mark as main queue to avoid a deadlock in `pqrs::gcd::dispatch_sync_on_main_queue` in destructor.
          pqrs::gcd::scoped_running_on_main_queue_marker marker;

          delete components_manager;
        }

        CFRunLoopStop(CFRunLoopGetCurrent());
      });
    });
  }

  components_manager = new krbn::kextd::components_manager();
  components_manager->async_start();

  CFRunLoopRun();

  krbn::components_manager_killer::terminate_shared_components_manager_killer();

  krbn::logger::get_logger()->info("karabiner_kextd is terminated.");

  return 0;
}
