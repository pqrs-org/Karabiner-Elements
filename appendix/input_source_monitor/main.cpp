#include "logger.hpp"
#include "monitor/input_source_monitor.hpp"
#include "thread_utility.hpp"
#include <Carbon/Carbon.h>

int main(int argc, char** argv) {
  pqrs::dispatcher::extra::initialize_shared_dispatcher();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  krbn::logger::get_logger().set_level(spdlog::level::off);

  for (int i = 0; i < 100; ++i) {
    // Check destructor working properly.
    krbn::input_source_monitor m;
    m.async_start();
  }

  krbn::logger::get_logger().set_level(spdlog::level::info);

  auto m = std::make_unique<krbn::input_source_monitor>();

  m->input_source_changed.connect([](auto&& input_source_identifiers) {
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

  m->async_start();

  // ------------------------------------------------------------

  CFRunLoopRun();

  // ------------------------------------------------------------

  m = nullptr;

  pqrs::dispatcher::extra::terminate_shared_dispatcher();

  return 0;
}
