#include "logger.hpp"
#include "monitor/event_tap_monitor.hpp"

int main(int argc, const char* argv[]) {
  pqrs::dispatcher::extra::initialize_shared_dispatcher();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto event_tap_monitor = std::make_unique<krbn::event_tap_monitor>();

  event_tap_monitor->caps_lock_state_changed.connect([](auto&& state) {
    krbn::logger::get_logger().info("caps_lock_state_changed {0}", state);
  });

  event_tap_monitor->pointing_device_event_arrived.connect([](auto&& event_type, auto&& event) {
    krbn::logger::get_logger().info("pointing_device_event_arrived {0} {1}",
                                    nlohmann::json(event_type).dump(),
                                    nlohmann::json(event).dump());
  });

  event_tap_monitor->async_start();

  CFRunLoopRun();

  event_tap_monitor = nullptr;

  pqrs::dispatcher::extra::terminate_shared_dispatcher();

  return 0;
}
