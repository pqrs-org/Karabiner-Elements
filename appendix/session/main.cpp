#include "monitor/console_user_id_monitor.hpp"
#include "session.hpp"
#include "thread_utility.hpp"
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto dispatcher = std::make_shared<krbn::dispatcher::dispatcher>();

  auto console_user_id_monitor = std::make_unique<krbn::console_user_id_monitor>(dispatcher);

  console_user_id_monitor->console_user_id_changed.connect([](auto&& uid) {
    if (uid) {
      std::cout << "console_user_id_changed: " << *uid << std::endl;
    } else {
      std::cout << "console_user_id_changed: none" << std::endl;
    }
  });

  console_user_id_monitor->async_start();

  CFRunLoopRun();

  console_user_id_monitor = nullptr;

  dispatcher->terminate();
  dispatcher = nullptr;

  return 0;
}
