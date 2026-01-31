#include "console_user_server/components_manager.hpp"
#include "console_user_server/migration.hpp"
#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "environment_variable_utility.hpp"
#include "filesystem_utility.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "process_utility.hpp"
#include "run_loop_thread_utility.hpp"
#include "services_utility.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/filesystem.hpp>

int main(int argc, const char* argv[]) {
  //
  // Initialize
  //

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_shared_run_loop_thread();

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  umask(0022);

  pqrs::osx::process_info::enable_sudden_termination();

  //
  // Load custom environment variables
  //

  auto environment_variables = krbn::environment_variable_utility::load_custom_environment_variables();

  //
  // Setup logger
  //

  if (!krbn::constants::get_user_log_directory().empty()) {
    krbn::logger::set_async_rotating_logger("console_user_server",
                                            krbn::constants::get_user_log_directory() / "console_user_server.log",
                                            pqrs::spdlog::filesystem::log_directory_perms_0700);
  }

  krbn::logger::get_logger()->info("version {0}", karabiner_version);

  //
  // Log custom environment variables
  //

  krbn::environment_variable_utility::log(environment_variables);

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
  // Get codesign
  //

  krbn::get_shared_codesign_manager()->log();

  //
  // Migrate old configuration file
  //

  krbn::console_user_server::migration::migrate_v1();

  //
  // Activate driver
  //

  system("/Applications/.Karabiner-VirtualHIDDevice-Manager.app/Contents/MacOS/Karabiner-VirtualHIDDevice-Manager forceActivate &");

  //
  // Register services
  //

  krbn::services_utility::bootout_old_agents();
  krbn::services_utility::register_core_daemons();
  krbn::services_utility::register_core_agents();

  //
  // Create directories
  //

  krbn::filesystem_utility::mkdir_user_directories();

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
