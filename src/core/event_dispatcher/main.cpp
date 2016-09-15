#include "constants.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "receiver.hpp"
#include "session.hpp"
#include <chrono>
#include <thread>

int main(int argc, const char* argv[]) {
  logger::get_logger().info("version {0}", karabiner_version);

  // ----------------------------------------
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  while (true) {
    try {
      receiver receiver;
      break;
    } catch (...) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

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

  CFRunLoopRun();
  return 0;
}
