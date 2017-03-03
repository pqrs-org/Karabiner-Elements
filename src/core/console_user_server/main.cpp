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
  krbn::thread_utility::register_main_thread();

  auto& logger = krbn::logger::get_logger();
  logger.info("version {0}", karabiner_version);

  if (!krbn::process_utility::lock_single_application_with_user_pid_file("karabiner_console_user_server.pid")) {
    std::string message("Exit since another process is running.");
    logger.info(message);
    std::cerr << message << std::endl;
    return 0;
  }

  auto version_monitor_ptr = std::make_unique<krbn::version_monitor>(logger, [] {
    exit(0);
  });

  krbn::migration::migrate_v1();

  krbn::filesystem::create_directory_with_intermediate_directories(krbn::constants::get_user_configuration_directory(), 0700);

  krbn::connection_manager manager(*version_monitor_ptr);

  CFRunLoopRun();

  version_monitor_ptr = nullptr;

  return 0;
}
