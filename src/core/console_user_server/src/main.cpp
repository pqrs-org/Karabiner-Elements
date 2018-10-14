#include "components_manager.hpp"
#include "constants.hpp"
#include "dispatcher.hpp"
#include "dispatcher_utility.hpp"
#include "filesystem.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "migration.hpp"
#include "process_utility.hpp"

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  // Setup logger

  if (!krbn::constants::get_user_log_directory().empty()) {
    krbn::logger::set_async_rotating_logger("console_user_server",
                                            krbn::constants::get_user_log_directory() + "/console_user_server.log",
                                            0700);
  }

  krbn::logger::get_logger().info("version {0}", karabiner_version);

  // Check another process

  if (!krbn::process_utility::lock_single_application_with_user_pid_file("karabiner_console_user_server.pid")) {
    krbn::logger::get_logger().info("Exit since another process is running.");
    exit(0);
  }

  // Migrate old configuration file

  krbn::migration::migrate_v1();

  // Create directories

  krbn::filesystem::create_directory_with_intermediate_directories(krbn::constants::get_user_configuration_directory(), 0700);
  krbn::filesystem::create_directory_with_intermediate_directories(krbn::constants::get_user_complex_modifications_assets_directory(), 0700);

  // Run components_manager

  auto components_manager = std::make_unique<krbn::components_manager>();

  CFRunLoopRun();

  components_manager = nullptr;

  krbn::logger::get_logger().info("karabiner_console_user_server is terminated.");

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
