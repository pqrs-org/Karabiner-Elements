#include "frontmost_application_observer.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"
#include <Carbon/Carbon.h>

static void callback(const char* bundle_identifier, const char* file_path) {
  krbn::logger::get_logger().info("callback");
  krbn::logger::get_logger().info("  bundle_identifier:{0}", bundle_identifier ? bundle_identifier : "(nullptr)");
  krbn::logger::get_logger().info("  file_path:{0}", file_path ? file_path : "(nullptr)");
}

int main(int argc, char** argv) {
  krbn::logger::get_logger().set_level(spdlog::level::off);

  for (int i = 0; i < 100; ++i) {
    // Check destructor working properly.
    krbn::frontmost_application_observer observer(callback);
  }

  krbn::logger::get_logger().set_level(spdlog::level::info);

  krbn::frontmost_application_observer observer(callback);
  CFRunLoopRun();
  return 0;
}
