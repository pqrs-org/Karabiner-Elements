#include "dispatcher_utility.hpp"
#include "logger.hpp"
#include "monitor/event_tap_monitor.hpp"

int main(int argc, const char* argv[]) {
  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto event_tap_monitor = std::make_unique<krbn::event_tap_monitor>();

  event_tap_monitor->pointing_device_event_arrived.connect([](auto&& event_type, auto&& event) {
    krbn::logger::get_logger()->info("pointing_device_event_arrived {0} {1}",
                                     nlohmann::json(event_type).dump(),
                                     nlohmann::json(event).dump());
  });

  event_tap_monitor->async_start();

  CFRunLoopRun();

  event_tap_monitor = nullptr;

  scoped_dispatcher_manager = nullptr;

  std::cout << "finished" << std::endl;

  return 0;
}
