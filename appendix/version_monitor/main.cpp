#include "dispatcher_utility.hpp"
#include "monitor/version_monitor.hpp"
#include "run_loop_thread_utility.hpp"
#include <iostream>

int main() {
  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::exit);

  signal(SIGINT, [](int) noexcept {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto monitor = std::make_unique<krbn::version_monitor>(krbn::constants::get_version_file_path());

  monitor->changed.connect([](auto&& version) {
    krbn::logger::get_logger()->info("changed");
  });

  monitor->async_start();

  CFRunLoopRun();

  monitor = nullptr;

  scoped_run_loop_thread_manager = nullptr;
  scoped_dispatcher_manager = nullptr;

  std::cout << "finished" << std::endl;

  return 0;
}
