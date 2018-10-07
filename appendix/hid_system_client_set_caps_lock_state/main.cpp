#include "hid_system_client.hpp"
#include "logger.hpp"

int main(int argc, const char* argv[]) {
  pqrs::dispatcher::extra::initialize_shared_dispatcher();

  if (getuid() != 0) {
    krbn::logger::get_logger().error("dump_caps_lock_state requires root privilege.");
  }

  auto client = std::make_unique<krbn::hid_system_client>();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  client->async_set_caps_lock_state(true);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  client->async_set_caps_lock_state(true);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  client->async_set_caps_lock_state(false);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  client = nullptr;

  pqrs::dispatcher::extra::terminate_shared_dispatcher();

  return 0;
}
