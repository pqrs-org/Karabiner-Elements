#include "grabber.hpp"

int main(int argc, const char* argv[]) {
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  const char* version = "version "
#include "version.hpp"
      ;
  logger::get_logger().info(version);

  userspace_connection_manager connection_manager;

  CFRunLoopRun();
  return 0;
}
