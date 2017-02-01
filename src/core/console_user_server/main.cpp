#include "connection_manager.hpp"
#include "constants.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "migration.hpp"
#include "process_utility.hpp"
#include "thread_utility.hpp"
#include "version_monitor.hpp"

int main(int argc, const char* argv[]) {
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);
  thread_utility::register_main_thread();

  logger::get_logger().info("version {0}", karabiner_version);

  if (!process_utility::lock_single_application_with_user_pid_file("karabiner_console_user_server.pid")) {
    std::string message("Exit since another process is running.");
    logger::get_logger().info(message);
    std::cerr << message << std::endl;
    return 0;
  }

  std::unique_ptr<version_monitor> version_monitor_ptr = std::make_unique<version_monitor>(logger::get_logger(), [] {
    exit(0);
  });

  migration::migrate_v1();

  filesystem::create_directory_with_intermediate_directories(constants::get_user_configuration_directory(), 0700);

  connection_manager manager(*version_monitor_ptr);

  CFRunLoopRun();

  version_monitor_ptr = nullptr;

  return 0;
}
