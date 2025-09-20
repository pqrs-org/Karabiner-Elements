#include "dispatcher_utility.hpp"
#include "grabber_client.hpp"
#include "json_utility.hpp"
#include <iostream>

namespace {
auto global_wait = pqrs::make_thread_wait();
}

int main(int argc, const char* argv[]) {
  std::cout << std::endl
            << "Type control-c to quit" << std::endl
            << std::endl
            << "To receive manipulator_environment_json from karabiner_grabber," << std::endl
            << "the process must be code-signed with the same Team ID as karabiner_grabber." << std::endl
            << std::endl;

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGINT, [](int) {
    global_wait->notify();
  });

  auto grabber_client = std::make_shared<krbn::grabber_client>("appendix_get_me");

  grabber_client->connected.connect([&] {
    grabber_client->async_connect_event_viewer();

    // At this stage, the connection is typically not verified yet,
    // so the `get_manipulator_environment` request is ignored.
    grabber_client->async_get_manipulator_environment();
  });

  grabber_client->connect_failed.connect([](auto&& error_code) {
    std::cerr << "connect_failed: " << error_code << std::endl;
  });

  grabber_client->received.connect([](auto&& buffer, auto&& sender_endpoint) {
    if (buffer) {
      if (buffer->empty()) {
        return;
      }

      try {
        nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
        switch (json.at("operation_type").get<krbn::operation_type>()) {
          case krbn::operation_type::manipulator_environment:
            std::cout << krbn::json_utility::dump(json.at("manipulator_environment")) << std::endl;
            break;

          default:
            break;
        }
      } catch (std::exception& e) {
        std::cerr << "received data is corrupted" << std::endl;
      }
    }
  });

  grabber_client->async_start();

  std::this_thread::sleep_for(std::chrono::seconds(1));

  grabber_client->async_get_manipulator_environment();

  std::thread thread([] {
    global_wait->wait_notice();
  });

  thread.join();

  grabber_client = nullptr;

  scoped_dispatcher_manager = nullptr;

  std::cout << "finished" << std::endl;

  return 0;
}
