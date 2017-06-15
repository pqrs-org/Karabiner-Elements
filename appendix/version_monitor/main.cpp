#include "boost_defs.hpp"

#include "version_monitor.hpp"

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  krbn::version_monitor monitor([] {
    krbn::logger::get_logger().info("version_changed_callback");
  });

  CFRunLoopRun();
  return 0;
}
