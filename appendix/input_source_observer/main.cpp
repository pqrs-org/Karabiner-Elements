#include "input_source_observer.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"
#include <Carbon/Carbon.h>

namespace {
void callback(const krbn::input_source_identifiers& input_source_identifiers) {
  krbn::logger::get_logger().info("callback");
  if (auto& v = input_source_identifiers.get_language()) {
    krbn::logger::get_logger().info("  language: {0}", *v);
  }
  if (auto& v = input_source_identifiers.get_input_source_id()) {
    krbn::logger::get_logger().info("  input_source_id: {0}", *v);
  }
  if (auto& v = input_source_identifiers.get_input_mode_id()) {
    krbn::logger::get_logger().info("  input_mode_id: {0}", *v);
  }
}
} // namespace

int main(int argc, char** argv) {
  krbn::thread_utility::register_main_thread();

  krbn::logger::get_logger().set_level(spdlog::level::off);

  for (int i = 0; i < 100; ++i) {
    // Check destructor working properly.
    krbn::input_source_observer observer(callback);
  }

  krbn::logger::get_logger().set_level(spdlog::level::info);

  krbn::input_source_observer input_source_observer(callback);

  CFRunLoopRun();
  return 0;
}
