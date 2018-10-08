#include "logger.hpp"
#include "monitor/frontmost_application_monitor.hpp"
#include "thread_utility.hpp"
#include <Carbon/Carbon.h>

namespace {
} // namespace

int main(int argc, char** argv) {
  pqrs::dispatcher::extra::initialize_shared_dispatcher();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  krbn::logger::get_logger().set_level(spdlog::level::off);

  for (int i = 0; i < 100; ++i) {
    // Check destructor working properly.
    auto m = std::make_unique<krbn::frontmost_application_monitor>();
    m->async_start();
  }

  krbn::logger::get_logger().set_level(spdlog::level::info);

  auto m = std::make_unique<krbn::frontmost_application_monitor>();

  m->frontmost_application_changed.connect([](auto&& bundle_identifier, auto&& file_path) {
    krbn::logger::get_logger().info("callback");
    krbn::logger::get_logger().info("  bundle_identifier:{0}", bundle_identifier);
    krbn::logger::get_logger().info("  file_path:{0}", file_path);
  });

  m->async_start();

  CFRunLoopRun();

  m = nullptr;

  pqrs::dispatcher::extra::terminate_shared_dispatcher();

  return 0;
}
