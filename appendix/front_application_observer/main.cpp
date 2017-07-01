#include "front_application_observer.h"
#include "logger.hpp"
#include "thread_utility.hpp"
#include <Carbon/Carbon.h>

static void callback(const char* bundle_identifier, const char* file_path) {
  krbn::logger::get_logger().info("callback");
  krbn::logger::get_logger().info("  bundle_identifier:{0}", bundle_identifier ? bundle_identifier : "(nullptr)");
  krbn::logger::get_logger().info("  file_path:{0}", file_path ? file_path : "(nullptr)");
}

int main(int argc, char** argv) {
  krbn_front_application_observer_initialize(callback);

  CFRunLoopRun();
  return 0;
}
