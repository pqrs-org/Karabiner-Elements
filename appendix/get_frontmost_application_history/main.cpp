#include "../../src/lib/libkrbn/include/libkrbn/libkrbn.h"
#include <iostream>
#include <pqrs/thread_wait.hpp>
#include <thread>
#include <unistd.h>

namespace {
void status_changed_callback(void) {
  switch (libkrbn_console_user_server_client_get_status()) {
    case libkrbn_console_user_server_client_status_connected:
      std::cerr << "console_user_server_client connected" << std::endl;

      libkrbn_console_user_server_client_async_get_frontmost_application_history();

      break;

    case libkrbn_console_user_server_client_status_connect_failed:
      std::cerr << "console_user_server_client connect_failed" << std::endl;
      break;

    case libkrbn_console_user_server_client_status_closed:
      std::cerr << "console_user_server_client closed" << std::endl;
      break;

    case libkrbn_console_user_server_client_status_none:
      break;
  }
}

void frontmost_application_history_received_callback(const char* json_string) {
  std::cout << json_string << std::endl;
}

auto global_wait = pqrs::make_thread_wait();
} // namespace

int main(int argc, const char* argv[]) {
  std::cout << std::endl
            << "Type control-c to quit" << std::endl
            << std::endl
            << "To receive frontmost_application_history from karabiner_console_user_server," << std::endl
            << "the process must be code-signed with the same Team ID as karabiner_console_user_server." << std::endl
            << std::endl;

  libkrbn_initialize();
  libkrbn_load_custom_environment_variables();

  signal(SIGINT, [](int) {
    global_wait->notify();
  });

  libkrbn_enable_console_user_server_client(geteuid(),
                                            "appendix_get_fah");

  libkrbn_register_console_user_server_client_status_changed_callback(status_changed_callback);
  libkrbn_register_console_user_server_client_frontmost_application_history_received_callback(frontmost_application_history_received_callback);

  libkrbn_console_user_server_client_async_start();

  std::this_thread::sleep_for(std::chrono::seconds(1));

  global_wait->wait_notice();

  libkrbn_terminate();

  std::cout << "finished" << std::endl;

  return 0;
} // namespace
