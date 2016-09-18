#include <iostream>

#include "session.hpp"

int main(int argc, const char* argv[]) {
  if (auto current_console_user_id = session::get_current_console_user_id()) {
    std::cout << "current_console_user_id: " << *current_console_user_id << std::endl;
  }

  return 0;
}
