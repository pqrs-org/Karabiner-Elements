#include "../../src/lib/libkrbn/include/libkrbn/libkrbn.h"
#include <iostream>
#include <pqrs/thread_wait.hpp>
#include <thread>

namespace {
void callback(const char* json_string) {
  std::cout << json_string << std::endl;
}

auto global_wait = pqrs::make_thread_wait();
} // namespace

int main(int argc, const char* argv[]) {
  std::cout << std::endl
            << "Type control-c to quit" << std::endl
            << std::endl;

  libkrbn_initialize();
  libkrbn_load_custom_environment_variables();

  signal(SIGINT, [](int) {
    global_wait->notify();
  });

  libkrbn_enable_core_service_client("appendix_get_sv");

  libkrbn_register_core_service_client_system_variables_received_callback(callback);

  libkrbn_core_service_client_async_start();

  std::jthread thread([](std::stop_token st) {
    while (!st.stop_requested()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));

      libkrbn_core_service_client_async_get_system_variables();
    }
  });

  global_wait->wait_notice();

  thread.request_stop();
  thread.join();

  libkrbn_terminate();

  std::cout << "finished" << std::endl;

  return 0;
} // namespace
