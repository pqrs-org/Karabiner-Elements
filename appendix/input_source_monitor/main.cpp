#include "dispatcher_utility.hpp"
#include "logger.hpp"
#include "monitor/input_source_monitor.hpp"

int main(int argc, char** argv) {
  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  krbn::logger::get_logger()->set_level(spdlog::level::off);

  for (int i = 0; i < 100; ++i) {
    // Check destructor working properly.
    krbn::input_source_monitor m;
    m.async_start();
  }

  krbn::logger::get_logger()->set_level(spdlog::level::info);

  auto m = std::make_unique<krbn::input_source_monitor>();

  m->input_source_changed.connect([](auto&& input_source_identifiers) {
    krbn::logger::get_logger()->info("callback");
    if (auto& v = input_source_identifiers.get_language()) {
      krbn::logger::get_logger()->info("  language: {0}", *v);
    }
    if (auto& v = input_source_identifiers.get_input_source_id()) {
      krbn::logger::get_logger()->info("  input_source_id: {0}", *v);
    }
    if (auto& v = input_source_identifiers.get_input_mode_id()) {
      krbn::logger::get_logger()->info("  input_mode_id: {0}", *v);
    }
  });

  m->async_start();

  // ------------------------------------------------------------

  CFRunLoopRun();

  // ------------------------------------------------------------

  m = nullptr;

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
