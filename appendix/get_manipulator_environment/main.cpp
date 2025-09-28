#include "../../src/lib/libkrbn/include/libkrbn/libkrbn.h"
#include <iostream>
#include <pqrs/thread_wait.hpp>
#include <thread>

namespace {
void status_changed_callback(void) {
  switch (libkrbn_grabber_client_get_status()) {
    case libkrbn_grabber_client_status_connected:
      std::cerr << "grabber_client connected" << std::endl;

      libkrbn_grabber_client_async_connect_event_viewer();

      // At this stage, the connection is typically not verified yet,
      // so the `get_manipulator_environment` request is ignored.
      libkrbn_grabber_client_async_get_manipulator_environment();

      break;

    case libkrbn_grabber_client_status_connect_failed:
      std::cerr << "grabber_client connect_failed" << std::endl;
      break;

    case libkrbn_grabber_client_status_closed:
      std::cerr << "grabber_client closed" << std::endl;
      break;

    case libkrbn_grabber_client_status_none:
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
            << "To receive manipulator_environment_json from karabiner_grabber," << std::endl
            << "the process must be code-signed with the same Team ID as karabiner_grabber." << std::endl
            << std::endl;

  libkrbn_initialize();
  libkrbn_load_custom_environment_variables();

  signal(SIGINT, [](int) {
    global_wait->notify();
  });

  libkrbn_enable_grabber_client("appendix_get_me");

  libkrbn_register_grabber_client_status_changed_callback(status_changed_callback);
  libkrbn_register_grabber_client_manipulator_environment_received_callback(manipulator_environment_received_callback);

  libkrbn_grabber_client_async_start();

  std::this_thread::sleep_for(std::chrono::seconds(1));

  libkrbn_grabber_client_async_get_manipulator_environment();

  global_wait->wait_notice();

  libkrbn_terminate();

  std::cout << "finished" << std::endl;

  return 0;
} // namespace
