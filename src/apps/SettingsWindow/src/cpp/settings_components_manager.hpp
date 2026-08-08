#pragma once

#include "settings_complex_modifications_assets_manager.hpp"
#include "settings_configuration_monitor.hpp"
#include "settings_console_user_server_client.hpp"
#include "settings_core_service_daemon_client.hpp"
#include "settings_log_monitor.hpp"
#include <unistd.h>

class settings_components_manager {
public:
  struct callbacks final {
    krbn_core_configuration_updated_t core_configuration_updated;
    krbn_log_messages_updated_t log_messages_updated;
    krbn_core_service_daemon_client_connected_devices_received_t connected_devices_received;
    krbn_core_service_daemon_client_system_variables_received_t system_variables_received;
    krbn_console_user_server_client_status_changed_t console_user_server_client_status_changed;
    krbn_console_user_server_client_settings_window_guidance_received_t settings_window_guidance_received;
  };

  settings_components_manager(const callbacks& callbacks)
      : configuration_monitor_(callbacks.core_configuration_updated),
        log_monitor_(callbacks.log_messages_updated),
        core_service_daemon_client_(
            [this](const auto& connected_devices) {
              configuration_monitor_.remember_connected_devices(connected_devices);
            },
            callbacks.connected_devices_received,
            callbacks.system_variables_received),
        console_user_server_client_(geteuid(),
                                    callbacks.console_user_server_client_status_changed,
                                    callbacks.settings_window_guidance_received) {
  }

  void async_start() {
    configuration_monitor_.start();
    core_service_daemon_client_.async_start();
    console_user_server_client_.async_start();
  }

  [[nodiscard]] std::shared_ptr<krbn::core_configuration::core_configuration> get_current_core_configuration() const {
    return configuration_monitor_.get_weak_core_configuration().lock();
  }

  [[nodiscard]] nlohmann::json reload_complex_modifications_assets() const {
    return complex_modifications_assets_manager_.reload_and_get_files_json();
  }

  void add_complex_modifications_rule_to_core_configuration_selected_profile(size_t file_index,
                                                                             size_t index) const {
    if (auto core_configuration = get_current_core_configuration()) {
      complex_modifications_assets_manager_.add_rule_to_core_configuration_selected_profile(file_index,
                                                                                            index,
                                                                                            *core_configuration);
    }
  }

  void erase_complex_modifications_asset_file(size_t index) const {
    complex_modifications_assets_manager_.erase_file(index);
  }

  void async_set_app_icon(int number) {
    core_service_daemon_client_.async_set_app_icon(number);
  }

  [[nodiscard]] bool console_user_server_client_connected() const {
    return console_user_server_client_.connected();
  }

private:
  settings_configuration_monitor configuration_monitor_;
  settings_complex_modifications_assets_manager complex_modifications_assets_manager_;
  settings_log_monitor log_monitor_;
  settings_core_service_daemon_client core_service_daemon_client_;
  settings_console_user_server_client console_user_server_client_;
};
