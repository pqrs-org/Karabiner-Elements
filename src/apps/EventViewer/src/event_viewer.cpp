#include "event_viewer.hpp"
#include "console_user_server_client.hpp"
#include "core_service_daemon_client.hpp"
#include "dispatcher_utility.hpp"
#include "environment_variable_utility.hpp"
#include "hat_switch_convert.hpp"
#include "hid_device_events_monitor.hpp"
#include "process_lifecycle_manager.hpp"
#include "run_loop_thread_utility.hpp"
#include "types.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <pqrs/gsl.hpp>
#include <pqrs/osx/iokit_hid_manager.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace {
std::atomic<krbn_core_service_connection_changed_callback> core_service_connection_changed_callback;
std::atomic<krbn_json_received_callback> manipulator_environment_received_callback;
std::atomic<krbn_json_received_callback> connected_devices_received_callback;
std::atomic<krbn_json_received_callback> frontmost_application_history_received_callback;
std::atomic<krbn_hid_value_monitor_stopped_callback> hid_value_monitor_stopped_callback;
std::atomic<krbn_hid_value_arrived_callback> hid_value_arrived_callback;
std::atomic<krbn_hid_input_report_arrived_callback> hid_input_report_arrived_callback;
std::atomic<krbn_hid_device_open_state_changed_callback> hid_device_open_state_changed_callback;
std::atomic<uint64_t> hid_input_report_capture_device_id{0};

std::shared_ptr<krbn::core_service_daemon_client> core_service_daemon_client;
std::shared_ptr<krbn::console_user_server_client> console_user_server_client;

class hid_value_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  hid_value_monitor(const hid_value_monitor&) = delete;

  hid_value_monitor()
      : dispatcher_client() {
    std::vector<pqrs::cf::cf_ptr<CFDictionaryRef>> matching_dictionaries{
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::keyboard),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::mouse),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::pointer),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::joystick),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::game_pad),

        // Headset
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::consumer,
            pqrs::hid::usage::consumer::consumer_control),

        // Special devices (e.g., VEC USB Footpedal INFINITY USB-3)
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::consumer,
            pqrs::hid::usage::consumer::programmable_buttons),
    };

    hid_manager_ = std::make_unique<pqrs::osx::iokit_hid_manager>(
        pqrs::dispatcher::extra::get_shared_dispatcher(),
        pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread(),
        matching_dictionaries);

    hid_manager_->device_matched.connect([this](auto&& registry_entry_id, auto&& device_ptr) {
      if (!device_ptr) {
        return;
      }

      auto device_id = krbn::make_device_id(registry_entry_id);
      auto device_properties = krbn::device_properties::make_device_properties(device_id,
                                                                               *device_ptr);
      auto monitor = std::make_shared<krbn::hid_device_events_monitor>(
          pqrs::dispatcher::extra::get_shared_dispatcher(),
          pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread(),
          *device_ptr,
          *device_properties,
          krbn::hid_device_events_monitor::configuration{
              // Some devices, such as ELECOM trackballs, produce additional input
              // values through the input report handler. Capture Raw Input Events
              // should observe only values delivered by IOHIDQueue, so disable the
              // handler here. Raw Input Records remains available through
              // input_report_observer.
              .enable_input_report_handler = false,
              .input_report_observer = [device_id](auto report_id, auto report) {
                if (hid_input_report_capture_device_id.load() != type_safe::get(device_id)) {
                  return;
                }

                if (auto callback = hid_input_report_arrived_callback.load()) {
                  callback(type_safe::get(device_id),
                           report_id,
                           report.data(),
                           report.size());
                }
              },
          });
      hid_device_events_monitors_.insert_or_assign(device_id, monitor);

      monitor->started.connect([device_id] {
        if (auto callback = hid_device_open_state_changed_callback.load()) {
          callback(type_safe::get(device_id), true);
        }
      });

      monitor->stopped.connect([this, device_id] {
        handle_monitor_stopped(device_id);

        if (auto callback = hid_device_open_state_changed_callback.load()) {
          callback(type_safe::get(device_id), false);
        }
      });

      monitor->values_arrived.connect([this, device_id](auto&& values) {
        handle_hid_values(device_id,
                          *values);
      });

      start_monitor_if_desired(device_id,
                               monitor);
    });

    hid_manager_->device_terminated.connect([this](auto&& registry_entry_id) {
      auto device_id = krbn::make_device_id(registry_entry_id);

      if (auto callback = hid_device_open_state_changed_callback.load()) {
        callback(type_safe::get(device_id), false);
      }

      requested_monitor_device_ids_.erase(device_id);
      auto pending_stop_erased = pending_stop_device_ids_.erase(device_id) > 0;
      hid_device_events_monitors_.erase(device_id);

      if (pending_stop_erased) {
        start_desired_monitors_if_ready();
      }

      krbn::hat_switch_converter::get_global_hat_switch_converter()->erase_device(device_id);
    });

    hid_manager_->error_occurred.connect([](auto&& message, auto&& kern_return) {
      krbn::logger::get_logger()->error("{0}: {1}", message, kern_return.to_string());
    });

    hid_manager_->async_start();
  }

  void async_set_capture_target(bool active,
                                std::optional<krbn::device_id> device_id,
                                std::function<void()> ready_callback) {
    enqueue_to_dispatcher([this, active, device_id, ready_callback = std::move(ready_callback)]() mutable {
      if (capture_active_ == active &&
          capture_device_id_ == device_id) {
        if (pending_stop_device_ids_.empty()) {
          ready_callback();
        } else {
          capture_target_ready_callback_ = std::move(ready_callback);
        }
        return;
      }

      capture_active_ = active;
      capture_device_id_ = device_id;
      capture_target_ready_callback_ = std::move(ready_callback);

      // A monitor stops asynchronously. Wait for every previous open request to
      // finish stopping before notifying the caller and opening the new target.
      for (const auto& requested_device_id : requested_monitor_device_ids_) {
        if (auto it = hid_device_events_monitors_.find(requested_device_id);
            it != std::end(hid_device_events_monitors_)) {
          pending_stop_device_ids_.insert(requested_device_id);
          it->second->async_stop();
        }
      }
      requested_monitor_device_ids_.clear();

      start_desired_monitors_if_ready();
    });
  }

  ~hid_value_monitor() override {
    detach_from_dispatcher([this] {
      hid_manager_ = nullptr;

      hid_device_events_monitors_.clear();

      if (auto callback = hid_value_monitor_stopped_callback.load()) {
        callback();
      }
    });
  }

private:
  [[nodiscard]] bool monitor_is_desired(krbn::device_id device_id) const {
    return capture_active_ &&
           (!capture_device_id_ ||
            *capture_device_id_ == device_id);
  }

  void start_monitor_if_desired(krbn::device_id device_id,
                                const pqrs::not_null_shared_ptr_t<krbn::hid_device_events_monitor>& monitor) {
    if (pending_stop_device_ids_.empty() &&
        monitor_is_desired(device_id) &&
        !requested_monitor_device_ids_.contains(device_id)) {
      requested_monitor_device_ids_.insert(device_id);
      monitor->async_start(kIOHIDOptionsTypeNone,
                           std::chrono::milliseconds(3000));
    }
  }

  void start_desired_monitors_if_ready() {
    if (!pending_stop_device_ids_.empty()) {
      return;
    }

    if (capture_target_ready_callback_) {
      auto callback = std::move(capture_target_ready_callback_);
      capture_target_ready_callback_ = nullptr;
      callback();
    }

    for (const auto& [device_id, monitor] : hid_device_events_monitors_) {
      start_monitor_if_desired(device_id,
                               monitor);
    }
  }

  void handle_monitor_stopped(krbn::device_id device_id) {
    if (pending_stop_device_ids_.erase(device_id) > 0) {
      start_desired_monitors_if_ready();
    }
  }

  void handle_hid_values(krbn::device_id device_id,
                         const std::vector<pqrs::osx::iokit_hid_value>& values) const {
    auto callback = hid_value_arrived_callback.load();
    if (!callback) {
      return;
    }

    for (const auto& v : values) {
      auto usage_page = v.get_usage_page();
      auto usage = v.get_usage();
      auto logical_max = v.get_logical_max();
      auto logical_min = v.get_logical_min();

      if (usage_page && usage && logical_max && logical_min) {
        std::optional<std::string> momentary_switch_event_json_string;
        std::optional<std::string> modifier_flag_name;

        if (krbn::momentary_switch_event::target(*usage_page, *usage)) {
          auto event = krbn::momentary_switch_event(*usage_page, *usage);
          auto json = nlohmann::json(event);
          if (json.is_null()) {
            json = nlohmann::json::object({
                {"usage_page", type_safe::get(*usage_page)},
                {"usage", type_safe::get(*usage)},
            });
          }
          momentary_switch_event_json_string = json.dump();

          if (auto modifier_flag = event.make_modifier_flag()) {
            if (auto name = krbn::get_modifier_flag_name(*modifier_flag)) {
              modifier_flag_name = std::string(*name);
            }
          }
        }

        callback(type_safe::get(device_id),
                 type_safe::get(*usage_page),
                 type_safe::get(*usage),
                 v.get_integer_value(),
                 momentary_switch_event_json_string
                     ? momentary_switch_event_json_string->c_str()
                     : nullptr,
                 modifier_flag_name
                     ? modifier_flag_name->c_str()
                     : nullptr);
      }
    }
  }

  std::unique_ptr<pqrs::osx::iokit_hid_manager> hid_manager_;
  std::unordered_map<krbn::device_id,
                     pqrs::not_null_shared_ptr_t<krbn::hid_device_events_monitor>>
      hid_device_events_monitors_;
  // Devices for which async_start has been requested, including monitors that
  // are still retrying IOHIDDeviceOpen.
  std::unordered_set<krbn::device_id> requested_monitor_device_ids_;
  // Devices whose stopped signal must arrive before the next target can start.
  std::unordered_set<krbn::device_id> pending_stop_device_ids_;
  bool capture_active_ = false;
  std::optional<krbn::device_id> capture_device_id_;
  std::function<void()> capture_target_ready_callback_;
};

std::shared_ptr<hid_value_monitor> global_hid_value_monitor;

class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager()
      : dispatcher_client() {
    //
    // core_service_daemon_client_
    //

    core_service_daemon_client_ = std::make_shared<krbn::core_service_daemon_client>();
    std::atomic_store(&core_service_daemon_client, core_service_daemon_client_);

    core_service_daemon_client_->connected.connect([this] {
      core_service_daemon_client_->async_observe_connected_devices();

      if (auto callback = core_service_connection_changed_callback.load()) {
        callback(true);
      }
    });
    core_service_daemon_client_->connect_failed.connect([](auto&&) {
      if (auto callback = core_service_connection_changed_callback.load()) {
        callback(false);
      }
    });
    core_service_daemon_client_->closed.connect([] {
      if (auto callback = core_service_connection_changed_callback.load()) {
        callback(false);
      }
    });
    core_service_daemon_client_->received.connect([](auto&& operation_type, auto&& json) {
      try {
        switch (operation_type) {
          case krbn::operation_type::manipulator_environment:
            if (auto callback = manipulator_environment_received_callback.load()) {
              auto value = krbn::json_utility::dump(json.at("manipulator_environment"));
              callback(value.c_str());
            }
            break;

          case krbn::operation_type::connected_devices:
            if (auto callback = connected_devices_received_callback.load()) {
              auto value = krbn::json_utility::dump(json.at("connected_devices"));
              callback(value.c_str());
            }
            break;

          default:
            break;
        }
      } catch (const std::exception&) {
        krbn::logger::get_logger()->error("core_service_daemon_client received data is corrupted");
      }
    });

    //
    // console_user_server_client_
    //

    console_user_server_client_ = std::make_shared<krbn::console_user_server_client>(geteuid());
    std::atomic_store(&console_user_server_client, console_user_server_client_);

    console_user_server_client_->received.connect([](auto&& operation_type, auto&& json) {
      if (operation_type != krbn::operation_type::frontmost_application_history) {
        return;
      }

      try {
        if (auto callback = frontmost_application_history_received_callback.load()) {
          auto value = krbn::json_utility::dump(json.at("frontmost_application_history"));
          callback(value.c_str());
        }
      } catch (const std::exception&) {
        krbn::logger::get_logger()->error("console_user_server_client received data is corrupted");
      }
    });
  }

  ~components_manager() override {
    detach_from_dispatcher([this] {
      std::atomic_store(&core_service_daemon_client,
                        std::shared_ptr<krbn::core_service_daemon_client>());
      std::atomic_store(&console_user_server_client,
                        std::shared_ptr<krbn::console_user_server_client>());
      std::atomic_store(&global_hid_value_monitor,
                        std::shared_ptr<hid_value_monitor>());
      core_service_daemon_client_ = nullptr;
      console_user_server_client_ = nullptr;
      hid_value_monitor_ = nullptr;

      if (auto callback = core_service_connection_changed_callback.load()) {
        callback(false);
      }
    });
  }

  void async_start() {
    core_service_daemon_client_->async_start();
    console_user_server_client_->async_start();
    hid_value_monitor_ = std::make_shared<hid_value_monitor>();
    std::atomic_store(&global_hid_value_monitor,
                      hid_value_monitor_);
  }

private:
  std::shared_ptr<krbn::core_service_daemon_client> core_service_daemon_client_;
  std::shared_ptr<krbn::console_user_server_client> console_user_server_client_;
  std::shared_ptr<hid_value_monitor> hid_value_monitor_;
};

std::shared_ptr<krbn::dispatcher_utility::scoped_dispatcher_manager> scoped_dispatcher_manager;
std::shared_ptr<krbn::run_loop_thread_utility::scoped_run_loop_thread_manager> scoped_run_loop_thread_manager;
} // namespace

void krbn_initialize(krbn_core_service_connection_changed_callback core_connection_callback,
                     krbn_json_received_callback manipulator_callback,
                     krbn_json_received_callback connected_devices_callback,
                     krbn_json_received_callback frontmost_application_callback,
                     krbn_hid_value_monitor_stopped_callback hid_monitor_stopped_callback,
                     krbn_hid_value_arrived_callback hid_callback,
                     krbn_hid_input_report_arrived_callback hid_input_report_callback,
                     krbn_hid_device_open_state_changed_callback hid_device_open_state_callback,
                     krbn_termination_completion_callback termination_completion_callback) {
  scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::exit);

  auto environment_variables = krbn::environment_variable_utility::load_custom_environment_variables();
  krbn::environment_variable_utility::log(environment_variables);

  core_service_connection_changed_callback = core_connection_callback;
  manipulator_environment_received_callback = manipulator_callback;
  connected_devices_received_callback = connected_devices_callback;
  frontmost_application_history_received_callback = frontmost_application_callback;
  hid_value_monitor_stopped_callback = hid_monitor_stopped_callback;
  hid_value_arrived_callback = hid_callback;
  hid_input_report_arrived_callback = hid_input_report_callback;
  hid_device_open_state_changed_callback = hid_device_open_state_callback;

  krbn::process_lifecycle_manager::initialize_shared_instance(
      krbn::process_lifecycle_manager::configuration{
          .components_manager_maker =
              [] {
                return std::make_unique<components_manager>();
              },
          .termination_completion_handler = termination_completion_callback,
      });
  krbn::process_lifecycle_manager::async_start();
}

bool krbn_async_request_termination(void) {
  return krbn::process_lifecycle_manager::async_request_termination();
}

void krbn_finalize() {
  hid_input_report_capture_device_id = 0;
  krbn::process_lifecycle_manager::terminate_shared_instance();
  scoped_run_loop_thread_manager = nullptr;
  scoped_dispatcher_manager = nullptr;
}

void krbn_core_service_async_get_manipulator_environment() {
  if (auto client = std::atomic_load(&core_service_daemon_client)) {
    client->async_get_manipulator_environment();
  }
}

void krbn_core_service_async_clear_user_variables() {
  if (auto client = std::atomic_load(&core_service_daemon_client)) {
    client->async_clear_user_variables();
  }
}

void krbn_set_hid_capture_target(bool active, uint64_t device_id) {
  // Close EventViewer's previous monitors before asking CoreService to change
  // which device it ignores. This serializes the IOHIDDevice ownership handoff.
  auto update_core_service = [device_id] {
    if (auto client = std::atomic_load(&core_service_daemon_client)) {
      client->async_temporarily_ignore_device(
          device_id == 0 ? std::nullopt : std::optional(krbn::device_id(device_id)));
    }
  };

  if (auto monitor = std::atomic_load(&global_hid_value_monitor)) {
    monitor->async_set_capture_target(
        active,
        device_id == 0 ? std::nullopt : std::optional(krbn::device_id(device_id)),
        std::move(update_core_service));
  } else {
    update_core_service();
  }
}

void krbn_set_hid_input_report_capture_device(uint64_t device_id) {
  hid_input_report_capture_device_id = device_id;
}

void krbn_console_user_server_async_get_frontmost_application_history() {
  if (auto client = std::atomic_load(&console_user_server_client)) {
    client->async_get_frontmost_application_history();
  }
}
