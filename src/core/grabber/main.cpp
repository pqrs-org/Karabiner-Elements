#include "connection_manager.hpp"
#include "constants.hpp"
#include "karabiner_version.h"
#include "notification_center.hpp"
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
  connection_manager connection_manager(device_grabber);

  notification_center::post_distributed_notification_to_all_sessions(constants::get_distributed_notification_grabber_is_launched());

  CFRunLoopRun();

  return 0;
}
