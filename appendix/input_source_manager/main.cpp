#include "input_source_manager.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"
#include <Carbon/Carbon.h>

int main(int argc, char** argv) {
  krbn::thread_utility::register_main_thread();

  krbn::input_source_manager input_source_manager;

  input_source_manager.select(krbn::input_source_selector(std::regex("fr"),
                                                          boost::none,
                                                          boost::none));

  for (int i = 0; i < 10; ++i) {
    std::cout << ".";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  std::cout << std::endl;

  input_source_manager.select(krbn::input_source_selector(std::regex("en"),
                                                          boost::none,
                                                          boost::none));

  std::cout << "type control-c" << std::endl;

  CFRunLoopRun();
  return 0;
}
