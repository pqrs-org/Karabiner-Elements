#include "connection_manager.hpp"
#include "constants.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "migration.hpp"
#include "thread_utility.hpp"
#include "version_monitor.hpp"

int main(int argc, const char* argv[]) {
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);
  thread_utility::register_main_thread();

  logger::get_logger().info("version {0}", karabiner_version);

  std::unique_ptr<version_monitor> version_monitor_ptr = std::make_unique<version_monitor>(logger::get_logger());

  migration::migrate_v1();

  system("open '/Library/Application Support/org.pqrs/Karabiner-Elements/updater/Karabiner-Elements.app'");

  filesystem::create_directory_with_intermediate_directories(constants::get_user_configuration_directory(), 0700);

  connection_manager manager(*version_monitor_ptr);

  CFRunLoopRun();

  version_monitor_ptr = nullptr;

  return 0;
}
