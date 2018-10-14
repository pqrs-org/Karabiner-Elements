#include "boost_defs.hpp"

#include "dispatcher_utility.hpp"
#include "monitor/log_monitor.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  std::vector<std::string> targets = {
      "/var/log/karabiner/grabber.log",
  };
  auto monitor = std::make_unique<krbn::log_monitor>(targets);

  monitor->new_log_line_arrived.connect([](auto&& line) {
    std::cout << line << std::endl;
  });

#if 1
  for (const auto& it : monitor->get_initial_lines()) {
    std::cout << it.second << std::endl;
  }
#endif

  monitor->async_start();

  // ------------------------------------------------------------

  CFRunLoopRun();

  // ------------------------------------------------------------

  monitor = nullptr;

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
