#include "connection_manager.hpp"
#include "constants.hpp"
#include "event_manipulator.hpp"
#include "karabiner_version.h"
#include "notification_center.hpp"
#include "thread_utility.hpp"
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
  thread_utility::register_main_thread();

  logger::get_logger().info("version {0}", karabiner_version);

  // load kexts
  system("/sbin/kextload '/Library/Application Support/org.pqrs/Karabiner-Elements/org.pqrs.driver.VirtualHIDManager.kext'");

  // make socket directory.
  mkdir(constants::get_tmp_directory(), 0755);
  chown(constants::get_tmp_directory(), 0, 0);
  chmod(constants::get_tmp_directory(), 0755);

  unlink(constants::get_grabber_socket_file_path());
  unlink(constants::get_event_dispatcher_socket_file_path());

  std::unique_ptr<manipulator::event_manipulator> event_manipulator_ptr = std::make_unique<manipulator::event_manipulator>();
  std::unique_ptr<device_grabber> device_grabber_ptr = std::make_unique<device_grabber>(*event_manipulator_ptr);
  connection_manager connection_manager(*event_manipulator_ptr, *device_grabber_ptr);

  notification_center::post_distributed_notification_to_all_sessions(constants::get_distributed_notification_grabber_is_launched());

  CFRunLoopRun();

  device_grabber_ptr = nullptr;
  event_manipulator_ptr = nullptr;

  return 0;
}
