#include "session.hpp"
#include "thread_utility.hpp"
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  while (true) {
    if (auto current_console_user_id = krbn::session::get_current_console_user_id()) {
      std::cout << "current_console_user_id: " << *current_console_user_id << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return 0;
}
