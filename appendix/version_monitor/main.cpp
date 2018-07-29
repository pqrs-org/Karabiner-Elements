#include "boost_defs.hpp"

#include "version_monitor.hpp"

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  auto monitor = krbn::version_monitor::get_shared_instance();

  monitor->changed.connect([] {
    krbn::logger::get_logger().info("changed");
  });

  monitor->start();

  CFRunLoopRun();
  return 0;
}
