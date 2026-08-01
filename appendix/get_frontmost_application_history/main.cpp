#include "console_user_server_client.hpp"
#include "dispatcher_utility.hpp"
#include "environment_variable_utility.hpp"
#include "json_utility.hpp"
#include "run_loop_thread_utility.hpp"
#include <iostream>
#include <pqrs/thread_wait.hpp>
#include <thread>
#include <unistd.h>

namespace {
auto global_wait = pqrs::make_thread_wait();
} // namespace

int main() {
  std::cout << std::endl
            << "Type control-c to quit" << std::endl
            << std::endl
            << "To receive frontmost_application_history from karabiner_console_user_server," << std::endl
            << "the process must be code-signed with the same Team ID as karabiner_console_user_server." << std::endl
            << std::endl;

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::exit);

  auto environment_variables = krbn::environment_variable_utility::load_custom_environment_variables();
  krbn::environment_variable_utility::log(environment_variables);

  signal(SIGINT, [](int) noexcept {
    global_wait->notify();
  });

  auto client = std::make_unique<krbn::console_user_server_client>(geteuid());

  client->connected.connect([&client] {
    std::cerr << "console_user_server_client connected" << std::endl;
    client->async_get_frontmost_application_history();
  });

  client->connect_failed.connect([](auto&&) {
    std::cerr << "console_user_server_client connect_failed" << std::endl;
  });

  client->closed.connect([] {
    std::cerr << "console_user_server_client closed" << std::endl;
  });

  client->received.connect([](auto&& operation_type, auto&& json) {
    if (operation_type == krbn::operation_type::frontmost_application_history) {
      std::cout << krbn::json_utility::dump(json.at("frontmost_application_history")) << std::endl;
    }
  });

  client->async_start();

  std::this_thread::sleep_for(std::chrono::seconds(1));

  global_wait->wait_notice();

  client = nullptr;

  std::cout << "finished" << std::endl;

  return 0;
}
