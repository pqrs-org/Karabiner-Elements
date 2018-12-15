#include "dispatcher_utility.hpp"
#include "input_source_manager.hpp"
#include "logger.hpp"

int main(int argc, char** argv) {
  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  krbn::input_source_manager input_source_manager;

  input_source_manager.select(krbn::input_source_selector(std::string("fr"),
                                                          std::nullopt,
                                                          std::nullopt));

  for (int i = 0; i < 10; ++i) {
    std::cout << i << "/9" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  input_source_manager.select(krbn::input_source_selector(std::string("en"),
                                                          std::nullopt,
                                                          std::nullopt));

  std::cout << "type control-c" << std::endl;

  CFRunLoopRun();

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
