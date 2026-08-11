#include "core_service_daemon_client.hpp"
#include "dispatcher_utility.hpp"
#include "environment_variable_utility.hpp"
#include "json_utility.hpp"
#include "process_lifecycle_manager.hpp"
#include "run_loop_thread_utility.hpp"
#include "termination_signal_monitor.hpp"
#include <iostream>
#include <pqrs/thread_wait.hpp>
#include <thread>

namespace {
auto global_wait = pqrs::make_thread_wait();

class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager()
      : dispatcher_client() {
    client_ = std::make_unique<krbn::core_service_daemon_client>();

    client_->connected.connect([this] {
      std::cerr << "core_service_daemon_client connected" << std::endl;
      client_->async_get_manipulator_environment();
    });

    client_->connect_failed.connect([](auto&&) {
      std::cerr << "core_service_daemon_client connect_failed" << std::endl;
    });

    client_->closed.connect([] {
      std::cerr << "core_service_daemon_client closed" << std::endl;
    });

    client_->received.connect([](auto&& operation_type, auto&& json) {
      if (operation_type == krbn::operation_type::manipulator_environment) {
        std::cout << krbn::json_utility::dump(json.at("manipulator_environment")) << std::endl;
      }
    });
  }

  ~components_manager() override {
    detach_from_dispatcher([this] {
      client_ = nullptr;
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      client_->async_start();
    });
  }

private:
  std::unique_ptr<krbn::core_service_daemon_client> client_;
};
} // namespace

int main() {
  std::cout << std::endl
            << "Type control-c to quit" << std::endl
            << std::endl
            << "To receive manipulator_environment_json from Karabiner-Core-Service.app," << std::endl
            << "the process must be code-signed with the same Team ID as Karabiner-Core-Service.app." << std::endl
            << std::endl;

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::exit);

  auto environment_variables = krbn::environment_variable_utility::load_custom_environment_variables();
  krbn::environment_variable_utility::log(environment_variables);

  krbn::process_lifecycle_manager::initialize_shared_instance(
      krbn::process_lifecycle_manager::configuration{
          .components_manager_maker =
              [] {
                return std::make_unique<components_manager>();
              },
          .termination_completion_handler = [] { global_wait->notify(); },
      });

  krbn::termination_signal_monitor signal_monitor([](int) {
    krbn::process_lifecycle_manager::async_request_termination();
  });

  krbn::process_lifecycle_manager::async_start();

  std::this_thread::sleep_for(std::chrono::seconds(1));

  global_wait->wait_notice();

  krbn::process_lifecycle_manager::terminate_shared_instance();

  std::cout << "finished" << std::endl;

  return 0;
}
