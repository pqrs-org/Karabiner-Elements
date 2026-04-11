#pragma once

#include "application_launcher.hpp"
#include "logger.hpp"
#include "services_utility.hpp"
#include "types/settings_window_alert_state.hpp"
#include <mutex>
#include <optional>
#include <pqrs/dispatcher.hpp>

namespace krbn {
namespace console_user_server {
class settings_window_alert_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  settings_window_alert_manager(void)
      : dispatcher_client(),
        current_alert_(settings_window_alert::none) {
  }

  ~settings_window_alert_manager(void) {
    detach_from_dispatcher();
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      // If Karabiner-Elements was manually terminated just before, the agents are in an unregistered state.
      // So we should enable them once before checking the status.
      services_utility::register_core_daemons();
      services_utility::register_core_agents();

      // When updating Karabiner-Elements, after the new version is installed, the daemons, agents, and Settings will restart.
      // To prevent alerts from appearing at that time, if the daemons or agents are enabled, wait for the timer to fire for the process startup check.
      // If either the daemons or agents are not enabled, usually when the daemons are not approved, trigger the check immediately after startup to show the alert right away.
      if (services_utility::core_daemons_enabled() &&
          services_utility::core_agents_enabled()) {
        enqueue_update_services_conditions();
      } else {
        update_services_conditions();
      }
    });
  }

  void async_update_core_service_daemon_state(const nlohmann::json& json) {
    enqueue_to_dispatcher([this, json] {
      update_core_service_daemon_state_conditions(json);
    });
  }

  settings_window_alert_state get_state(void) const {
    auto state = settings_window_alert_state();
    state.set_current_alert(get_current_alert());
    state.set_alert_context(get_alert_context());
    state.set_core_service_daemon_state(get_core_service_daemon_state());

    return state;
  }

private:
  settings_window_alert get_current_alert(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return current_alert_;
  }

  settings_window_alert_context get_alert_context(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto context = settings_window_alert_context();
    context.set_services_enabled(services_enabled_);
    context.set_core_daemons_running(core_daemons_running_);
    context.set_core_agents_running(core_agents_running_);

    auto services_waiting_seconds = 0;
    if (!(core_daemons_running_ && core_agents_running_)) {
      services_waiting_seconds =
          static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(
                               std::chrono::steady_clock::now() - services_waiting_started_at_)
                               .count());
    }
    context.set_services_waiting_seconds(services_waiting_seconds);

    return context;
  }

  nlohmann::json get_core_service_daemon_state(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return core_service_deamon_state_;
  }

  void update_core_service_daemon_state_conditions(const nlohmann::json& json) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      core_service_deamon_state_ = json;
    }

    doctor_alert_ =
        !json.value("karabiner_json_parse_error_message", std::string()).empty();
    virtual_hid_keyboard_type_not_set_ = json.value("virtual_hid_keyboard_type_not_set", false);

    update_optional_bool(virtual_hid_device_service_client_not_connected_alert_,
                         get_optional_inverted_bool(json,
                                                    "virtual_hid_device_service_client_connected"));
    update_optional_bool(driver_not_activated_alert_,
                         get_optional_inverted_bool(json,
                                                    "driver_activated"));
    update_driver_connected(get_optional_bool(json,
                                              "driver_connected"));
    update_optional_bool(driver_version_mismatched_alert_,
                         get_optional_bool(json,
                                           "driver_version_mismatched"));
    update_optional_bool(input_monitoring_permissions_alert_,
                         get_optional_inverted_bool(json,
                                                    "hid_device_open_permitted"));
    update_optional_bool(accessibility_alert_,
                         get_optional_inverted_bool(json,
                                                    "accessibility_process_trusted"));

    update_current_alert();
  }

  void update_services_conditions(void) {
    auto services_enabled =
        services_utility::core_daemons_enabled() &&
        services_utility::core_agents_enabled();
    auto core_daemons_running = services_utility::core_daemons_running();
    auto core_agents_running = services_utility::core_agents_running();
    auto services_running = core_daemons_running && core_agents_running;

    services_enabled_ = services_enabled;
    core_daemons_running_ = core_daemons_running;
    core_agents_running_ = core_agents_running;

    if (!previous_services_running_ ||
        *previous_services_running_ != services_running) {
      previous_services_running_ = services_running;

      if (!services_running) {
        services_waiting_started_at_ = std::chrono::steady_clock::now();
      }
    }

    services_not_running_alert_ = !services_running;

    if (!services_running) {
      services_utility::register_core_daemons();
      services_utility::register_core_agents();
    }

    update_current_alert();

    enqueue_update_services_conditions();
  }

  void enqueue_update_services_conditions(void) {
    enqueue_to_dispatcher(
        [this] {
          update_services_conditions();
        },
        when_now() + std::chrono::seconds(3));
  }

  void update_driver_connected(const std::optional<bool>& value) {
    if (driver_connected_ == value) {
      return;
    }

    driver_connected_ = value;
    ++driver_connected_generation_;

    if (!value) {
      update_optional_bool(driver_not_connected_alert_,
                           std::nullopt);
      return;
    }

    if (*value) {
      update_optional_bool(driver_not_connected_alert_,
                           false);
      return;
    }

    // `driver_connected` always transitions `nullopt -> false -> true` during normal startup.
    // If we immediately surface `driver_not_connected_alert_` when it becomes false,
    // SettingsWindow would show a transient alert while the driver is still connecting.
    // Therefore, false is treated as a pending state here, and we only show the alert
    // if it remains false after a delay.
    update_optional_bool(driver_not_connected_alert_,
                         false);
    enqueue_evaluate_driver_not_connected(driver_connected_generation_);
  }

  void enqueue_evaluate_driver_not_connected(std::size_t generation) {
    enqueue_to_dispatcher(
        [this, generation] {
          // Ignore stale delayed checks that were scheduled for an older state.
          if (driver_connected_generation_ != generation) {
            return;
          }

          if (driver_connected_ == false) {
            update_optional_bool(driver_not_connected_alert_,
                                 true);
          }
        },
        when_now() + std::chrono::seconds(3));
  }

  static std::optional<bool> get_optional_bool(const nlohmann::json& json,
                                               const std::string& key) {
    if (!json.contains(key) || json.at(key).is_null()) {
      return std::nullopt;
    }

    return json.at(key).get<bool>();
  }

  static std::optional<bool> get_optional_inverted_bool(const nlohmann::json& json,
                                                        const std::string& key) {
    if (auto value = get_optional_bool(json, key)) {
      return !*value;
    }

    return std::nullopt;
  }

  void update_optional_bool(std::optional<bool>& target,
                            const std::optional<bool>& value) {
    if (target == value) {
      return;
    }

    target = value;
    update_current_alert();
  }

  void update_current_alert(void) {
    auto new_current_alert = make_current_alert();
    auto previous_alert = settings_window_alert::none;

    {
      std::lock_guard<std::mutex> lock(mutex_);

      if (current_alert_ == new_current_alert) {
        return;
      }

      previous_alert = current_alert_;
      current_alert_ = new_current_alert;
    }

    logger::get_logger()->info("settings_window_alert changed: {0} -> {1}",
                               nlohmann::json(previous_alert).get<std::string>(),
                               nlohmann::json(new_current_alert).get<std::string>());

    if (new_current_alert != settings_window_alert::none) {
      application_launcher::launch_settings();
    }
  }

  settings_window_alert make_current_alert(void) const {
    if (doctor_alert_) {
      return settings_window_alert::doctor;
    }
    if (virtual_hid_keyboard_type_not_set_) {
      return settings_window_alert::settings;
    }
    if (services_not_running_alert_) {
      return settings_window_alert::services_not_running;
    }
    if (input_monitoring_permissions_alert_ == true) {
      return settings_window_alert::input_monitoring_permissions;
    }
    if (accessibility_alert_ == true) {
      return settings_window_alert::accessibility;
    }
    if (virtual_hid_device_service_client_not_connected_alert_ == true) {
      return settings_window_alert::virtual_hid_device_service_client_not_connected;
    }
    if (driver_version_mismatched_alert_ == true) {
      return settings_window_alert::driver_version_mismatched;
    }
    if (driver_not_activated_alert_ == true) {
      return settings_window_alert::driver_not_activated;
    }
    if (driver_not_connected_alert_ == true) {
      return settings_window_alert::driver_not_connected;
    }

    return settings_window_alert::none;
  }

  mutable std::mutex mutex_;
  settings_window_alert current_alert_;

  bool doctor_alert_ = false;
  bool virtual_hid_keyboard_type_not_set_ = false;
  bool services_not_running_alert_ = false;
  bool services_enabled_ = true;
  bool core_daemons_running_ = true;
  bool core_agents_running_ = true;
  std::optional<bool> input_monitoring_permissions_alert_;
  std::optional<bool> accessibility_alert_;
  std::optional<bool> virtual_hid_device_service_client_not_connected_alert_;
  std::optional<bool> driver_version_mismatched_alert_;
  std::optional<bool> driver_not_activated_alert_;
  std::optional<bool> driver_not_connected_alert_;
  std::optional<bool> driver_connected_;
  std::size_t driver_connected_generation_ = 0;
  std::optional<bool> previous_services_running_;
  std::chrono::steady_clock::time_point services_waiting_started_at_ = std::chrono::steady_clock::now();
  nlohmann::json core_service_deamon_state_ = nlohmann::json::object();
};
} // namespace console_user_server
} // namespace krbn
