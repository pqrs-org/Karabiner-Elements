#include "boost_defs.hpp"

#include "hid_system_client.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <boost/bind.hpp>
#include <iostream>
#include <mach/mach_time.h>

namespace {
class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("dump_caps_lock_state", true);
    }
    return *logger;
  }
};
}

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  if (getuid() != 0) {
    logger::get_logger().error("dump_caps_lock_state requires root privilege.");
  }

  krbn::hid_system_client client(logger::get_logger());
  if (auto state = client.get_caps_lock_state()) {
    std::cout << "state: " << *state << std::endl;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  client.set_caps_lock_state(true);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  if (auto state = client.get_caps_lock_state()) {
    std::cout << "state: " << *state << std::endl;
  }

  client.set_caps_lock_state(true);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  if (auto state = client.get_caps_lock_state()) {
    std::cout << "state: " << *state << std::endl;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  client.set_caps_lock_state(false);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  if (auto state = client.get_caps_lock_state()) {
    std::cout << "state: " << *state << std::endl;
  }

  return 0;
}
