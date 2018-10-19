#include "dispatcher_utility.hpp"
#include "hid_system_client.hpp"
#include "logger.hpp"

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto client = std::make_unique<krbn::hid_system_client>();
  client->caps_lock_state_changed.connect([](auto&& state) {
    if (!state) {
      krbn::logger::get_logger().info("caps_lock_state_changed: boost::none");
    } else {
      krbn::logger::get_logger().info("caps_lock_state_changed: {0}", *state);
    }
  });
  client->async_start_caps_lock_check_timer(std::chrono::milliseconds(100));

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  client->async_set_caps_lock_state(true);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  client->async_set_caps_lock_state(true);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  client->async_set_caps_lock_state(false);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  CFRunLoopRun();

  client = nullptr;

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
