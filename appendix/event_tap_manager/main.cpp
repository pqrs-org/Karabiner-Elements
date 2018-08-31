#include "event_tap_manager.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto event_tap_manager = std::make_unique<krbn::event_tap_manager>();

  event_tap_manager->caps_lock_state_changed.connect([](auto&& state) {
    krbn::logger::get_logger().info("caps_lock_state_changed {0}", state);
  });

  event_tap_manager->pointing_device_event_arrived.connect([](auto&& event_type, auto&& event) {
    krbn::logger::get_logger().info("pointing_device_event_arrived {0}", event_type);
  });

  event_tap_manager->async_start();

  CFRunLoopRun();

  event_tap_manager = nullptr;

  return 0;
}
