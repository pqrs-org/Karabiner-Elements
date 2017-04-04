#include "boost_defs.hpp"

#include <iostream>

#include "log_monitor.hpp"

namespace {
void new_log_line_callback(const std::string& line) {
  std::cout << line << std::endl;
}
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  std::vector<std::string> targets = {
      "/var/log/karabiner/grabber.log",
  };
  log_monitor monitor(targets, new_log_line_callback);

#if 1
  for (const auto& it : monitor.get_initial_lines()) {
    std::cout << it.second << std::endl;
  }
#endif

  monitor.start();

  CFRunLoopRun();
  return 0;
}
