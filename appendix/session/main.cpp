#include "monitor/console_user_id_monitor.hpp"
#include "session.hpp"
#include "thread_utility.hpp"
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  krbn::console_user_id_monitor console_user_id_monitor;

  console_user_id_monitor.console_user_id_changed.connect([](auto&& uid) {
    if (uid) {
      std::cout << "console_user_id_changed: " << *uid << std::endl;
    } else {
      std::cout << "console_user_id_changed: none" << std::endl;
    }
  });

  console_user_id_monitor.async_start();

  CFRunLoopRun();

  return 0;
}
