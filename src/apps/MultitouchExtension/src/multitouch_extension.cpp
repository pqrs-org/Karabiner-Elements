#include "multitouch_extension.hpp"
#include "core_service_daemon_client.hpp"
#include "dispatcher_utility.hpp"
#include "environment_variable_utility.hpp"
#include "process_lifecycle_manager.hpp"
#include "run_loop_thread_utility.hpp"
#include <atomic>
#include <memory>

namespace {
std::atomic<krbn_core_service_connected_changed_callback> connected_changed_callback;
std::shared_ptr<krbn::core_service_daemon_client> core_service_daemon_client;

void notify_connected_changed(bool value) {
  if (auto callback = connected_changed_callback.load()) {
    callback(value);
  }
}

class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager()
      : dispatcher_client() {
    client_ = std::make_shared<krbn::core_service_daemon_client>();
    std::atomic_store(&core_service_daemon_client, client_);

    client_->connected.connect([this] {
      client_->async_connect_multitouch_extension();
      notify_connected_changed(true);
    });

    client_->connect_failed.connect([](auto&&) {
      notify_connected_changed(false);
    });

    client_->closed.connect([] {
      notify_connected_changed(false);
    });
  }

  ~components_manager() override {
    detach_from_dispatcher([this] {
      // Stop new C API calls from acquiring the client before disconnecting
      // callbacks that capture this components_manager.
      std::atomic_store(&core_service_daemon_client,
                        std::shared_ptr<krbn::core_service_daemon_client>());

      if (client_) {
        client_->unregister_callbacks_and_detach();
      }

      client_ = nullptr;
      notify_connected_changed(false);
    });
  }

  void async_start() {
    client_->async_start();
  }

private:
  std::shared_ptr<krbn::core_service_daemon_client> client_;
};

std::shared_ptr<krbn::dispatcher_utility::scoped_dispatcher_manager> scoped_dispatcher_manager;
std::shared_ptr<krbn::run_loop_thread_utility::scoped_run_loop_thread_manager> scoped_run_loop_thread_manager;
} // namespace

void krbn_initialize(krbn_core_service_connected_changed_callback callback,
                     krbn_termination_completion_callback termination_completion_callback) {
  scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::exit);

  auto environment_variables = krbn::environment_variable_utility::load_custom_environment_variables();
  krbn::environment_variable_utility::log(environment_variables);

  connected_changed_callback = callback;

  krbn::process_lifecycle_manager::initialize_shared_instance(
      krbn::process_lifecycle_manager::configuration{
          .components_manager_maker =
              [] {
                return std::make_unique<components_manager>();
              },
          .termination_completion_handler = termination_completion_callback,
          .system_will_sleep_delay = std::chrono::seconds(1),
      });
  krbn::process_lifecycle_manager::async_start();
}

bool krbn_async_request_termination(void) {
  return krbn::process_lifecycle_manager::async_request_termination();
}

void krbn_finalize(void) {
  krbn::process_lifecycle_manager::terminate_shared_instance();
  scoped_run_loop_thread_manager = nullptr;
  scoped_dispatcher_manager = nullptr;
}

bool krbn_core_service_async_set_variables(krbn_multitouch_extension_variables variables) {
  if (auto client = std::atomic_load(&core_service_daemon_client)) {
    client->async_set_variables(nlohmann::json::object({
        {"multitouch_extension_finger_count_upper_quarter_area", variables.finger_count_upper_quarter_area},
        {"multitouch_extension_finger_count_lower_quarter_area", variables.finger_count_lower_quarter_area},
        {"multitouch_extension_finger_count_left_quarter_area", variables.finger_count_left_quarter_area},
        {"multitouch_extension_finger_count_right_quarter_area", variables.finger_count_right_quarter_area},
        {"multitouch_extension_finger_count_upper_half_area", variables.finger_count_upper_half_area},
        {"multitouch_extension_finger_count_lower_half_area", variables.finger_count_lower_half_area},
        {"multitouch_extension_finger_count_left_half_area", variables.finger_count_left_half_area},
        {"multitouch_extension_finger_count_right_half_area", variables.finger_count_right_half_area},
        {"multitouch_extension_finger_count_total", variables.finger_count_total},
        {"multitouch_extension_palm_count_upper_half_area", variables.palm_count_upper_half_area},
        {"multitouch_extension_palm_count_lower_half_area", variables.palm_count_lower_half_area},
        {"multitouch_extension_palm_count_left_half_area", variables.palm_count_left_half_area},
        {"multitouch_extension_palm_count_right_half_area", variables.palm_count_right_half_area},
        {"multitouch_extension_palm_count_total", variables.palm_count_total},
    }));

    return true;
  }

  return false;
}
