#include "constants.hpp"
#include "karabiner_version.h"
#include "logger.hpp"
#include "receiver.hpp"
#include "session.hpp"
#include <chrono>
#include <memory>
#include <thread>

int main(int argc, const char* argv[]) {
  if (getuid() != 0) {
    std::cerr << "fatal: karabiner_event_dispatcher requires root privilege." << std::endl;
    exit(1);
  }

  // ----------------------------------------
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  chmod("/var/log/karabiner/event_dispatcher_log.txt", 0644);

  logger::get_logger().info("version {0}", karabiner_version);

  // ----------------------------------------
  std::unique_ptr<receiver> r;

  while (true) {
    try {
      r = std::make_unique<receiver>();
      break;
    } catch (...) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  // ----------------------------------------
  boost::optional<uid_t> uid;
  while (true) {
    uid = session::get_current_console_user_id();
    if (uid) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  if (setuid(*uid) != 0) {
    logger::get_logger().error("setuid error @ {0}", __PRETTY_FUNCTION__);
    return 1;
  }

  // ----------------------------------------
  CFRunLoopRun();
  return 0;
}
