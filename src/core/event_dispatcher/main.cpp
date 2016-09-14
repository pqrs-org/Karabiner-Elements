#include "constants.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "session.hpp"

int main(int argc, const char* argv[]) {
  logger::get_logger().info("version {0}", karabiner_version);

  // ----------------------------------------
  if (getuid() != 0) {
    logger::get_logger().error("karabiner_event_dispatcher requires root privilege.");
    return 1;
  }

  auto uid = session::get_current_console_user_id();
  if (!uid) {
    logger::get_logger().error("session::get_current_console_user_id() error @ {0}", __PRETTY_FUNCTION__);
    return 1;
  }

  if (setuid(*uid) != 0) {
    logger::get_logger().error("setuid error @ {0}", __PRETTY_FUNCTION__);
    return 1;
  }

  // ----------------------------------------
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  CFRunLoopRun();
  return 0;
}
