#include "console_user_server/components_manager.hpp"
#include "console_user_server/migration.hpp"
#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "karabiner_version.h"
#include "launchctl_utility.hpp"
#include "logger.hpp"
#include "process_utility.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/filesystem.hpp>

int main(int argc, const char* argv[]) {
  //
  // Initialize
  //

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  //
  // Setup logger
  //

  if (!krbn::constants::get_user_log_directory().empty()) {
    krbn::logger::set_async_rotating_logger("console_user_server",
                                            krbn::constants::get_user_log_directory() + "/console_user_server.log",
                                            0700);
  }

  krbn::logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Check another process
  //

  if (!krbn::process_utility::lock_single_application_with_user_pid_file("karabiner_console_user_server.pid")) {
    auto message = "Exit since another process is running.";
    krbn::logger::get_logger()->info(message);
    std::cerr << message << std::endl;
    return 1;
  }

  //
  // Manage launchctl
  //

  krbn::launchctl_utility::manage_session_monitor();
  krbn::launchctl_utility::manage_observer_agent();
  krbn::launchctl_utility::manage_grabber_agent();
  krbn::launchctl_utility::manage_console_user_server(true);

  //
  // Migrate old configuration file
  //

  krbn::console_user_server::migration::migrate_v1();

  //
  // Create directories
  //

  pqrs::filesystem::create_directory_with_intermediate_directories(krbn::constants::get_user_configuration_directory(), 0700);
  pqrs::filesystem::create_directory_with_intermediate_directories(krbn::constants::get_user_complex_modifications_assets_directory(), 0700);

  //
  // Run components_manager
  //

  krbn::components_manager_killer::initialize_shared_components_manager_killer();

  // We have to use raw pointer (not smart pointer) to delete it in `dispatch_async`.
  krbn::console_user_server::components_manager* components_manager = nullptr;

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

  components_manager = new krbn::console_user_server::components_manager();
  components_manager->async_start();

  CFRunLoopRun();

  krbn::components_manager_killer::terminate_shared_components_manager_killer();

  krbn::logger::get_logger()->info("karabiner_console_user_server is terminated.");

  return 0;
}
