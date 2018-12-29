#include "dispatcher_utility.hpp"
#include "monitor/version_monitor.hpp"

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto monitor = std::make_unique<krbn::version_monitor>(krbn::constants::get_version_file_path());

  monitor->changed.connect([](auto&& version) {
    krbn::logger::get_logger()->info("changed");
  });

  monitor->async_start();

  CFRunLoopRun();

  monitor = nullptr;

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
