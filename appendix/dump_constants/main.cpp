#include "constants.hpp"
#include "thread_utility.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  std::cout << "get_user_configuration_directory: " << krbn::constants::get_user_configuration_directory() << std::endl;
  std::cout << "get_user_complex_modifications_assets_directory: " << krbn::constants::get_user_complex_modifications_assets_directory() << std::endl;
  std::cout << "get_user_data_directory: " << krbn::constants::get_user_data_directory() << std::endl;
  std::cout << "get_user_log_directory: " << krbn::constants::get_user_log_directory() << std::endl;
  std::cout << "get_user_pid_directory: " << krbn::constants::get_user_pid_directory() << std::endl;
  std::cout << "get_user_core_configuration_file_path: " << krbn::constants::get_user_core_configuration_file_path() << std::endl;

  std::cout << std::endl;

  return 0;
}
