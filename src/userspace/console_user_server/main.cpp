#include "connection_manager.hpp"
#include "karabiner_version.h"
#include "logger.hpp"

int main(int argc, const char* argv[]) {
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  logger::get_logger().info("version {0}", karabiner_version);

  connection_manager manager;

  CFRunLoopRun();
  return 0;
}
