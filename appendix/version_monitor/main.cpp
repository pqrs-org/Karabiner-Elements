#include "monitor/version_monitor.hpp"

int main(int argc, const char* argv[]) {
  pqrs::dispatcher::extra::initialize_shared_dispatcher();

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

  pqrs::dispatcher::extra::terminate_shared_dispatcher();

  return 0;
}
