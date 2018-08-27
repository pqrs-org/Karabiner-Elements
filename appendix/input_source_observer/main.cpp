#include "input_source_observer.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"
#include <Carbon/Carbon.h>

int main(int argc, char** argv) {
  krbn::thread_utility::register_main_thread();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  krbn::logger::get_logger().set_level(spdlog::level::off);

  for (int i = 0; i < 100; ++i) {
    // Check destructor working properly.
    krbn::input_source_observer observer;
    observer.async_start();
  }

  krbn::logger::get_logger().set_level(spdlog::level::info);

  krbn::input_source_observer observer;

  observer.input_source_changed.connect([](auto&& input_source_identifiers) {
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
  });

  observer.async_start();

  CFRunLoopRun();

  return 0;
}
