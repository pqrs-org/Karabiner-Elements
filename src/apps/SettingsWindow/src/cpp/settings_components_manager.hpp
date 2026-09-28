#pragma once

#include "logger.hpp"
#include "settings_configuration_monitor.hpp"
#include "settings_console_user_server_client.hpp"
#include "settings_core_service_daemon_client.hpp"
#include "settings_log_monitor.hpp"
#include <mutex>
#include <unistd.h>

class settings_components_manager {
public:
  struct callbacks final {
    krbn_core_configuration_updated_t core_configuration_updated;
    krbn_core_configuration_load_state_changed_t core_configuration_load_state_changed;
    krbn_log_messages_updated_t log_messages_updated;
    krbn_core_service_daemon_client_connected_devices_received_t connected_devices_received;
    krbn_core_service_daemon_client_system_variables_received_t system_variables_received;
    krbn_console_user_server_client_status_changed_t console_user_server_client_status_changed;
    krbn_console_user_server_client_settings_window_guidance_received_t settings_window_guidance_received;
  };

  settings_components_manager(const callbacks& callbacks)
      : configuration_monitor_(callbacks.core_configuration_updated,
                               callbacks.core_configuration_load_state_changed),
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

  ~settings_components_manager() {
    unregister_callbacks_and_detach();
  }

  void unregister_callbacks_and_detach() {
    std::call_once(unregister_callbacks_and_detach_once_, [this] {
      core_service_daemon_client_.unregister_callbacks_and_detach();
      console_user_server_client_.unregister_callbacks_and_detach();
      configuration_monitor_.unregister_callbacks_and_detach();
      log_monitor_.unregister_callbacks_and_detach();
    });
  }

  void async_start() {
    configuration_monitor_.start();
    core_service_daemon_client_.async_start();
    console_user_server_client_.async_start();
  }

  [[nodiscard]] pqrs::not_null_shared_ptr_t<settings_configuration_store> get_configuration_store() const {
    return configuration_monitor_.get_configuration_store();
  }

  void async_set_app_icon(int number) {
    core_service_daemon_client_.async_set_app_icon(number);
  }

  [[nodiscard]] bool console_user_server_client_connected() const {
    return console_user_server_client_.connected();
  }

private:
  settings_configuration_monitor configuration_monitor_;
  settings_log_monitor log_monitor_;
  settings_core_service_daemon_client core_service_daemon_client_;
  settings_console_user_server_client console_user_server_client_;
  std::once_flag unregister_callbacks_and_detach_once_;
};
