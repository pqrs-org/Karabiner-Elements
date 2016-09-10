#include "karabiner_version.h"
#include "userspace_connection_manager.hpp"
#include <iostream>
#include <unistd.h>

int main(int argc, const char* argv[]) {
  if (getuid() != 0) {
    std::cerr << "fatal: karabiner_grabber requires root privilege." << std::endl;
    exit(1);
  }

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  logger::get_logger().info("version {0}", karabiner_version);

  device_grabber device_grabber;
  userspace_connection_manager connection_manager(device_grabber);

  CFRunLoopRun();

  return 0;
}
