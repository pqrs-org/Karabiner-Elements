#include <chrono>
#include <iostream>
#include <thread>

#include "session.hpp"

int main(int argc, const char* argv[]) {
  while (true) {
    if (auto current_console_user_id = session::get_current_console_user_id()) {
      std::cout << "current_console_user_id: " << *current_console_user_id << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return 0;
}
