#include "frontmost_application_observer.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"
#include <Carbon/Carbon.h>

namespace {
} // namespace

int main(int argc, char** argv) {
  krbn::thread_utility::register_main_thread();

  krbn::logger::get_logger().set_level(spdlog::level::off);

  for (int i = 0; i < 100; ++i) {
    // Check destructor working properly.
    krbn::frontmost_application_observer observer;
    observer.async_start();
  }

  krbn::logger::get_logger().set_level(spdlog::level::info);

  krbn::frontmost_application_observer observer;

  observer.frontmost_application_changed.connect([](auto&& bundle_identifier, auto&& file_path) {
    krbn::logger::get_logger().info("callback");
    krbn::logger::get_logger().info("  bundle_identifier:{0}", bundle_identifier);
    krbn::logger::get_logger().info("  file_path:{0}", file_path);
  });

  observer.async_start();

  CFRunLoopRun();

  return 0;
}
