#include "console_user_server_client.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"
#include "time_utility.hpp"
#include "virtual_hid_device_client.hpp"

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();
  auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

  auto client = std::make_shared<krbn::console_user_server_client>(dispatcher);
  std::weak_ptr<krbn::console_user_server_client> weak_client = client;

  client->connected.connect([weak_client] {
    std::string shell_command = "open /Applications/Safari.app";
    if (auto c = weak_client.lock()) {
      c->async_shell_command_execution(shell_command);
    }
  });

  client->connect_failed.connect([](auto&& error_code) {
    krbn::logger::get_logger().error("Failed to connect");
  });

  client->async_start();

  // ------------------------------------------------------------

  CFRunLoopRun();

  // ------------------------------------------------------------

  client = nullptr;

  dispatcher->terminate();
  dispatcher = nullptr;

  return 0;
}
