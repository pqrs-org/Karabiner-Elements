#include "logger.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "thread_utility.hpp"
#include "time_utility.hpp"
#include "virtual_hid_device_client.hpp"

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  auto client = krbn::console_user_server_client::get_shared_instance();

  client->connected.connect([client] {
    std::string shell_command = "open /Applications/Safari.app";
    client->shell_command_execution(shell_command);
  });

  client->connect_failed.connect([](auto&& error_code) {
    krbn::logger::get_logger().error("Failed to connect");
  });

  client->start();

  CFRunLoopRun();

  return 0;
}
