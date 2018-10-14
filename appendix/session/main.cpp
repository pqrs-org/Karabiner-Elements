#include "dispatcher_utility.hpp"
#include "monitor/console_user_id_monitor.hpp"
#include "session.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto console_user_id_monitor = std::make_unique<krbn::console_user_id_monitor>();

  console_user_id_monitor->console_user_id_changed.connect([](auto&& uid) {
    if (uid) {
      std::cout << "console_user_id_changed: " << *uid << std::endl;
    } else {
      std::cout << "console_user_id_changed: none" << std::endl;
    }
  });

  for (int i = 0; i < 10; ++i) {
    console_user_id_monitor->async_start();
    console_user_id_monitor->async_stop();
  }
  console_user_id_monitor->async_start();

  CFRunLoopRun();

  console_user_id_monitor = nullptr;

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
