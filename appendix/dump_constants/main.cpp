#include "constants.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  std::cout << "get_user_configuration_directory: " << constants::get_user_configuration_directory() << std::endl;

  std::cout << std::endl;

  return 0;
}
