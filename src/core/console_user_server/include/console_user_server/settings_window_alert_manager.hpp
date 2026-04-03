#pragma once

#include "application_launcher.hpp"
#include "constants.hpp"
#include "json_utility.hpp"
#include "logger.hpp"
#include "monitor/configuration_monitor.hpp"
#include "services_utility.hpp"
#include "types/settings_window_alert.hpp"
#include <fstream>
#include <mutex>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/file_monitor.hpp>

namespace krbn {
namespace console_user_server {
class settings_window_alert_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  nod::signal<void(settings_window_alert)> current_alert_changed;

  settings_window_alert_manager(void)
      : dispatcher_client(),
        current_alert_(settings_window_alert::none),
        configuration_monitor_(std::make_unique<krbn::configuration_monitor>(
            constants::get_user_core_configuration_file_path(),
            geteuid(),
            krbn::core_configuration::error_handling::loose)),
        core_service_state_file_monitor_(std::make_unique<pqrs::osx::file_monitor>(
            weak_dispatcher_,
            std::vector<std::string>{constants::get_core_service_state_json_file_path().string()})) {
    configuration_monitor_->core_configuration_updated.connect([this](auto&&) {
      update_configuration_conditions();
    });

    core_service_state_file_monitor_->file_changed.connect([this](auto&&, auto&&) {
      update_core_service_state_conditions();
    });
  }

  ~settings_window_alert_manager(void) {
    detach_from_dispatcher([this] {
      core_service_state_file_monitor_ = nullptr;
      configuration_monitor_ = nullptr;
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      configuration_monitor_->async_start();
      core_service_state_file_monitor_->async_start();

      // If Karabiner-Elements was manually terminated just before, the agents are in an unregistered state.
      // So we should enable them once before checking the status.
      services_utility::register_core_daemons();
      services_utility::register_core_agents();

      update_configuration_conditions();
      update_core_service_state_conditions();

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

  settings_window_alert get_current_alert(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return current_alert_;
  }

private:
  void update_configuration_conditions(void) {
    doctor_alert_ = !configuration_monitor_->get_parse_error_message().empty();

    settings_alert_ = false;
    if (auto c = configuration_monitor_->get_core_configuration()) {
      settings_alert_ = c->get_selected_profile().get_virtual_hid_keyboard()->get_keyboard_type_v2().empty();
    }

    update_current_alert();
  }

  void update_core_service_state_conditions(void) {
    std::ifstream input(constants::get_core_service_state_json_file_path());
    if (!input) {
      update_optional_bool(virtual_hid_device_service_client_not_connected_alert_,
                           std::nullopt);
      update_optional_bool(driver_not_activated_alert_,
                           std::nullopt);
      update_driver_connected(std::nullopt);
      update_optional_bool(driver_version_mismatched_alert_,
                           std::nullopt);
      update_optional_bool(input_monitoring_permissions_alert_,
                           std::nullopt);
      update_optional_bool(accessibility_alert_,
                           std::nullopt);
      return;
    }

    try {
      auto json = json_utility::parse_jsonc(input);

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
    } catch (const std::exception& e) {
      logger::get_logger()->error("parse error in {0}: {1}",
                                  constants::get_core_service_state_json_file_path().string(),
                                  e.what());
    }
  }

  void update_services_conditions(void) {
    auto services_running =
        services_utility::core_daemons_running() &&
        services_utility::core_agents_running();

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

    current_alert_changed(new_current_alert);
  }

  settings_window_alert make_current_alert(void) const {
    if (doctor_alert_) {
      return settings_window_alert::doctor;
    }
    if (settings_alert_) {
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
  bool settings_alert_ = false;
  bool services_not_running_alert_ = false;
  std::optional<bool> input_monitoring_permissions_alert_;
  std::optional<bool> accessibility_alert_;
  std::optional<bool> virtual_hid_device_service_client_not_connected_alert_;
  std::optional<bool> driver_version_mismatched_alert_;
  std::optional<bool> driver_not_activated_alert_;
  std::optional<bool> driver_not_connected_alert_;
  std::optional<bool> driver_connected_;
  std::size_t driver_connected_generation_ = 0;

  std::unique_ptr<krbn::configuration_monitor> configuration_monitor_;
  std::unique_ptr<pqrs::osx::file_monitor> core_service_state_file_monitor_;
};
} // namespace console_user_server
} // namespace krbn
