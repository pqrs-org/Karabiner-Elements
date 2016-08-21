#include "grabber.hpp"
#include "karabiner_version.h"
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

  // load kexts
  system("/sbin/kextload /Library/Extensions/org.pqrs.driver.VirtualHIDKeyboard.kext");
  system("/sbin/kextload /Library/Extensions/org.pqrs.driver.VirtualHIDPointing.kext");
  system("/sbin/kextload /Library/Extensions/org.pqrs.driver.VirtualHIDManager.kext");

  userspace_connection_manager connection_manager;
  CFRunLoopRun();

  return 0;
}
