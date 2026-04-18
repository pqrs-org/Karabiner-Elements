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
enum class alert_state {
  unknown,
  inactive,
  active,
};

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

  void async_update_core_service_daemon_state(const core_service_daemon_state& state) {
    enqueue_to_dispatcher([this, state] {
      update_core_service_daemon_state_conditions(state);
    });
  }

  settings_window_alert_state get_state(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto state = settings_window_alert_state();

    state.set_current_alert(current_alert_);

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

    state.set_alert_context(context);
    state.set_core_service_daemon_state(core_service_deamon_state_);

    return state;
  }

private:
  void update_core_service_daemon_state_conditions(const core_service_daemon_state& state) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      core_service_deamon_state_ = state;
    }

    auto iohid_listen_event_allowed = std::optional<bool>();
    auto accessibility_process_trusted = std::optional<bool>();
    if (auto permission_check_result = state.get_bundle_permission_check_result()) {
      iohid_listen_event_allowed = permission_check_result->get_iohid_listen_event_allowed();
      accessibility_process_trusted = permission_check_result->get_accessibility_process_trusted();
    }

    update_karabiner_json_parse_error_message_alert_state(state.get_karabiner_json_parse_error_message());
    virtual_hid_keyboard_type_not_set_ = state.get_virtual_hid_keyboard_type_not_set().value_or(false);

    update_alert_state(virtual_hid_device_service_client_not_connected_alert_state_,
                       make_inverted_alert_state(state.get_virtual_hid_device_service_client_connected()));
    update_alert_state(driver_not_activated_alert_state_,
                       make_inverted_alert_state(state.get_driver_activated()));
    update_driver_connected(state.get_driver_connected());
    update_alert_state(driver_version_mismatched_alert_state_,
                       make_alert_state(state.get_driver_version_mismatched()));
    update_alert_state(input_monitoring_permissions_alert_state_,
                       make_inverted_alert_state(iohid_listen_event_allowed));
    update_alert_state(accessibility_alert_state_,
                       make_inverted_alert_state(accessibility_process_trusted));

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

  template <typename F>
  void enqueue_delayed_evaluation(std::size_t generation,
                                  std::size_t settings_window_alert_manager::* generation_member,
                                  pqrs::dispatcher::duration delay,
                                  F&& evaluator) {
    enqueue_to_dispatcher(
        [this, generation, generation_member, evaluator = std::forward<F>(evaluator)] {
          if (this->*generation_member != generation) {
            return;
          }

          evaluator();
        },
        when_now() + delay);
  }

  void update_driver_connected(const std::optional<bool>& value) {
    if (driver_connected_ == value) {
      return;
    }

    driver_connected_ = value;
    ++driver_connected_generation_;

    if (!value) {
      update_alert_state(driver_not_connected_alert_state_,
                         alert_state::unknown);
      return;
    }

    if (*value) {
      update_alert_state(driver_not_connected_alert_state_,
                         alert_state::inactive);
      return;
    }

    // `driver_connected` always transitions `nullopt -> false -> true` during normal startup.
    // If we immediately surface `driver_not_connected_alert_` when it becomes false,
    // SettingsWindow would show a transient alert while the driver is still connecting.
    // Therefore, false is treated as a pending state here, and we only show the alert
    // if it remains false after a delay.
    update_alert_state(driver_not_connected_alert_state_,
                       alert_state::inactive);

    enqueue_delayed_evaluation(
        driver_connected_generation_,
        &settings_window_alert_manager::driver_connected_generation_,
        pqrs::dispatcher::duration(std::chrono::seconds(3)),
        [this] {
          if (driver_connected_ == false) {
            update_alert_state(driver_not_connected_alert_state_,
                               alert_state::active);
          }
        });
  }

  void update_karabiner_json_parse_error_message_alert_state(const std::string& value) {
    if (karabiner_json_parse_error_message_ == value) {
      return;
    }

    karabiner_json_parse_error_message_ = value;
    ++karabiner_json_parse_error_message_generation_;

    if (value.empty()) {
      update_alert_state(doctor_alert_state_,
                         alert_state::inactive);
      return;
    }

    // karabiner.json can be observed in the middle of an external split write.
    // Delay surfacing the parse error so a transient partial-write state does not open Settings.
    update_alert_state(doctor_alert_state_,
                       alert_state::inactive);
    enqueue_delayed_evaluation(
        karabiner_json_parse_error_message_generation_,
        &settings_window_alert_manager::karabiner_json_parse_error_message_generation_,
        pqrs::dispatcher::duration(std::chrono::seconds(3)),
        [this] {
          if (!karabiner_json_parse_error_message_.empty()) {
            update_alert_state(doctor_alert_state_,
                               alert_state::active);
          }
        });
  }

  static alert_state make_alert_state(const std::optional<bool>& value) {
    if (!value) {
      return alert_state::unknown;
    }

    return *value ? alert_state::active : alert_state::inactive;
  }

  static alert_state make_inverted_alert_state(const std::optional<bool>& value) {
    if (!value) {
      return alert_state::unknown;
    }

    return *value ? alert_state::inactive : alert_state::active;
  }

  void update_alert_state(alert_state& target,
                          alert_state value) {
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
    if (doctor_alert_state_ == alert_state::active) {
      return settings_window_alert::doctor;
    }
    if (virtual_hid_keyboard_type_not_set_) {
      return settings_window_alert::settings;
    }
    if (services_not_running_alert_) {
      return settings_window_alert::services_not_running;
    }
    // On macOS 26, Accessibility permission may also cover Input Monitoring permission.
    // (Note that on macOS 14, Input Monitoring permission is still required separately even if Accessibility is granted.)
    // Therefore, since Accessibility permission is requested first,
    // the Accessibility alert is also displayed with higher priority than the Input Monitoring alert.
    if (accessibility_alert_state_ == alert_state::active) {
      return settings_window_alert::accessibility;
    }
    if (input_monitoring_permissions_alert_state_ == alert_state::active) {
      return settings_window_alert::input_monitoring_permissions;
    }
    if (virtual_hid_device_service_client_not_connected_alert_state_ == alert_state::active) {
      return settings_window_alert::virtual_hid_device_service_client_not_connected;
    }
    if (driver_version_mismatched_alert_state_ == alert_state::active) {
      return settings_window_alert::driver_version_mismatched;
    }
    if (driver_not_activated_alert_state_ == alert_state::active) {
      return settings_window_alert::driver_not_activated;
    }
    if (driver_not_connected_alert_state_ == alert_state::active) {
      return settings_window_alert::driver_not_connected;
    }

    return settings_window_alert::none;
  }

  mutable std::mutex mutex_;
  settings_window_alert current_alert_;

  alert_state doctor_alert_state_ = alert_state::inactive;
  alert_state input_monitoring_permissions_alert_state_ = alert_state::unknown;
  alert_state accessibility_alert_state_ = alert_state::unknown;
  alert_state virtual_hid_device_service_client_not_connected_alert_state_ = alert_state::unknown;
  alert_state driver_version_mismatched_alert_state_ = alert_state::unknown;
  alert_state driver_not_activated_alert_state_ = alert_state::unknown;
  alert_state driver_not_connected_alert_state_ = alert_state::unknown;

  bool virtual_hid_keyboard_type_not_set_ = false;
  bool services_not_running_alert_ = false;
  bool services_enabled_ = true;
  bool core_daemons_running_ = true;
  bool core_agents_running_ = true;
  std::optional<bool> driver_connected_;
  std::size_t driver_connected_generation_ = 0;
  std::string karabiner_json_parse_error_message_;
  std::size_t karabiner_json_parse_error_message_generation_ = 0;
  std::optional<bool> previous_services_running_;
  std::chrono::steady_clock::time_point services_waiting_started_at_ = std::chrono::steady_clock::now();
  core_service_daemon_state core_service_deamon_state_;
};
} // namespace console_user_server
} // namespace krbn
