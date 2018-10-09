#include "iopm_client.hpp"
#include "logger.hpp"

int main(int argc, const char* argv[]) {
  pqrs::dispatcher::extra::initialize_shared_dispatcher();

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

  pqrs::dispatcher::extra::terminate_shared_dispatcher();

  return 0;
}
