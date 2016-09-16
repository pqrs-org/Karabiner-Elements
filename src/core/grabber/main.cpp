#include "connection_manager.hpp"
#include "constants.hpp"
#include "event_dispatcher_manager.hpp"
#include "karabiner_version.h"
#include "notification_center.hpp"
#include <iostream>
#include <unistd.h>

int main(int argc, const char* argv[]) {
  if (getuid() != 0) {
    std::cerr << "fatal: karabiner_grabber requires root privilege." << std::endl;
    exit(1);
  }

  // ----------------------------------------
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  logger::get_logger().info("version {0}", karabiner_version);

  std::unique_ptr<event_dispatcher_manager> event_dispatcher_manager_ptr = std::make_unique<event_dispatcher_manager>();
  std::unique_ptr<device_grabber> device_grabber_ptr = std::make_unique<device_grabber>(*event_dispatcher_manager_ptr);
  connection_manager connection_manager(*event_dispatcher_manager_ptr, *device_grabber_ptr);

  notification_center::post_distributed_notification_to_all_sessions(constants::get_distributed_notification_grabber_is_launched());

  CFRunLoopRun();

  device_grabber_ptr = nullptr;
  event_dispatcher_manager_ptr = nullptr;

  return 0;
}
