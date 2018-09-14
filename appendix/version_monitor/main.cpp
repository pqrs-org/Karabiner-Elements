#include "boost_defs.hpp"

#include "monitor/version_monitor.hpp"
#include "thread_utility.hpp"

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto monitor = std::make_unique<krbn::version_monitor>(krbn::constants::get_version_file_path());

  monitor->changed.connect([](auto&& version) {
    krbn::logger::get_logger().info("changed");
  });

  monitor->async_start();

  CFRunLoopRun();

  monitor = nullptr;

  return 0;
}
