#include "boost_defs.hpp"

#include "monitor/log_monitor.hpp"
#include <Carbon/Carbon.h>
#include <iostream>

int main(int argc, const char* argv[]) {
  pqrs::dispatcher::extra::initialize_shared_dispatcher();

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

  pqrs::dispatcher::extra::terminate_shared_dispatcher();

  return 0;
}
