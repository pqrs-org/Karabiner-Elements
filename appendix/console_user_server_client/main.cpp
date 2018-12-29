#include "console_user_server_client.hpp"
#include "dispatcher_utility.hpp"
#include "logger.hpp"
#include "time_utility.hpp"
#include "virtual_hid_device_client.hpp"

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto client = std::make_shared<krbn::console_user_server_client>();
  std::weak_ptr<krbn::console_user_server_client> weak_client = client;

  client->connected.connect([weak_client] {
    std::string shell_command = "open /Applications/Safari.app";
    if (auto c = weak_client.lock()) {
      c->async_shell_command_execution(shell_command);
    }
  });

  client->connect_failed.connect([](auto&& error_code) {
    krbn::logger::get_logger()->error("Failed to connect");
  });

  client->async_start();

  // ------------------------------------------------------------

  CFRunLoopRun();

  // ------------------------------------------------------------

  client = nullptr;

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
