#include "dispatcher_utility.hpp"
#include "iopm_client.hpp"
#include "logger.hpp"

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto client = std::make_unique<krbn::iopm_client>();

  client->system_power_event_arrived.connect([](auto&& message_type) {
    krbn::logger::get_logger().info("callback message_type:{0}", message_type);
  });

  client->async_start();

  // ------------------------------------------------------------

  CFRunLoopRun();

  // ------------------------------------------------------------

  client = nullptr;

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
