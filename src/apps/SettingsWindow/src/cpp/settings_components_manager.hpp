#pragma once

#include "settings_complex_modifications_assets_manager.hpp"
#include "settings_configuration_monitor.hpp"
#include "settings_console_user_server_client.hpp"
#include "settings_core_service_daemon_client.hpp"
#include "settings_dispatcher_client.hpp"
#include "settings_file_monitors.hpp"
#include "settings_log_monitor.hpp"
#include <pqrs/gsl.hpp>

class settings_components_manager {
public:
  settings_components_manager()
      : dispatcher_client_(std::make_shared<settings_dispatcher_client>()) {
  }

  void enqueue_callback(void (*callback)()) {
    dispatcher_client_->enqueue(callback);
  }

  //
  // configuration_monitor_
  //

  void enable_configuration_monitor() {
    if (!configuration_monitor_) {
      configuration_monitor_ = std::make_shared<settings_configuration_monitor>();
    }
  }

  [[nodiscard]] std::shared_ptr<settings_configuration_monitor> get_settings_configuration_monitor() const {
    return configuration_monitor_;
  }

  [[nodiscard]] std::shared_ptr<krbn::core_configuration::core_configuration> get_current_core_configuration() const {
    if (auto m = configuration_monitor_) {
      return m->get_weak_core_configuration().lock();
    }
    return nullptr;
  }

  //
  // complex_modifications_assets_manager_;
  //

  void enable_complex_modifications_assets_manager() {
    if (!complex_modifications_assets_manager_) {
      complex_modifications_assets_manager_ = std::make_unique<settings_complex_modifications_assets_manager>();
    }
  }

  [[nodiscard]] std::shared_ptr<settings_complex_modifications_assets_manager> get_complex_modifications_assets_manager() const {
    return complex_modifications_assets_manager_;
  }

  //
  // file_monitors_
  //

  void enable_file_monitors() {
    if (!file_monitors_) {
      file_monitors_ = std::make_shared<settings_file_monitors>();
    }
  }

  [[nodiscard]] std::shared_ptr<settings_file_monitors> get_settings_file_monitors() const {
    return file_monitors_;
  }

  //
  // log_monitor_
  //

  void enable_log_monitor() {
    if (!log_monitor_) {
      log_monitor_ = std::make_shared<settings_log_monitor>();
    }
  }

  void disable_log_monitor() {
    log_monitor_ = nullptr;
  }

  [[nodiscard]] std::shared_ptr<settings_log_monitor> get_settings_log_monitor() const {
    return log_monitor_;
  }

  [[nodiscard]] std::shared_ptr<std::deque<std::string>> get_current_log_lines() const {
    if (auto m = log_monitor_) {
      return m->get_lines();
    }
    return nullptr;
  }

  //
  // core_service_daemon_client_
  //

  void enable_core_service_daemon_client() {
    if (!core_service_daemon_client_) {
      core_service_daemon_client_ = std::make_shared<settings_core_service_daemon_client>();
    }
  }

  [[nodiscard]] std::shared_ptr<settings_core_service_daemon_client> get_settings_core_service_daemon_client() const {
    return core_service_daemon_client_;
  }

  //
  // console_user_server_client_
  //

  void enable_console_user_server_client(uid_t uid) {
    if (!console_user_server_client_) {
      console_user_server_client_ = std::make_shared<settings_console_user_server_client>(uid);
    }
  }

  [[nodiscard]] std::shared_ptr<settings_console_user_server_client> get_settings_console_user_server_client() const {
    return console_user_server_client_;
  }

private:
  pqrs::not_null_shared_ptr_t<settings_dispatcher_client> dispatcher_client_;
  std::shared_ptr<settings_configuration_monitor> configuration_monitor_;
  std::shared_ptr<settings_complex_modifications_assets_manager> complex_modifications_assets_manager_;
  std::shared_ptr<settings_file_monitors> file_monitors_;
  std::shared_ptr<settings_log_monitor> log_monitor_;
  std::shared_ptr<settings_core_service_daemon_client> core_service_daemon_client_;
  std::shared_ptr<settings_console_user_server_client> console_user_server_client_;
};
