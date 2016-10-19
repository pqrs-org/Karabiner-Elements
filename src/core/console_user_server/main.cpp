#include "connection_manager.hpp"
#include "constants.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "thread_utility.hpp"

int main(int argc, const char* argv[]) {
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);
  thread_utility::register_main_thread();

  logger::get_logger().info("version {0}", karabiner_version);

  system("open '/Library/Application Support/org.pqrs/Karabiner-Elements/updater/Karabiner-Elements.app'");

  mkdir(constants::get_configuration_directory(), 0700);

  connection_manager manager;

  CFRunLoopRun();
  return 0;
}
