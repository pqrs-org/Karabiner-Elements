#include "../../src/lib/libkrbn/include/libkrbn/libkrbn.h"
#include <iostream>
#include <pqrs/thread_wait.hpp>
#include <thread>

namespace {
void status_changed_callback(void) {
  switch (libkrbn_core_service_client_get_status()) {
    case libkrbn_core_service_client_status_connected:
      std::cerr << "core_service_client connected" << std::endl;

      libkrbn_core_service_client_async_get_manipulator_environment();

      break;

    case libkrbn_core_service_client_status_connect_failed:
      std::cerr << "core_service_client connect_failed" << std::endl;
      break;

    case libkrbn_core_service_client_status_closed:
      std::cerr << "core_service_client closed" << std::endl;
      break;

    case libkrbn_core_service_client_status_none:
      break;
  }
}

void manipulator_environment_received_callback(const char* json_string) {
  std::cout << json_string << std::endl;
}

auto global_wait = pqrs::make_thread_wait();
} // namespace

int main(int argc, const char* argv[]) {
  std::cout << std::endl
            << "Type control-c to quit" << std::endl
            << std::endl
            << "To receive manipulator_environment_json from Karabiner-Core-Service.app," << std::endl
            << "the process must be code-signed with the same Team ID as Karabiner-Core-Service.app." << std::endl
            << std::endl;

  libkrbn_initialize();
  libkrbn_load_custom_environment_variables();

  signal(SIGINT, [](int) {
    global_wait->notify();
  });

  libkrbn_enable_core_service_client("appendix_get_me");

  libkrbn_register_core_service_client_status_changed_callback(status_changed_callback);
  libkrbn_register_core_service_client_manipulator_environment_received_callback(manipulator_environment_received_callback);

  libkrbn_core_service_client_async_start();

  std::this_thread::sleep_for(std::chrono::seconds(1));

  global_wait->wait_notice();

  libkrbn_terminate();

  std::cout << "finished" << std::endl;

  return 0;
} // namespace
