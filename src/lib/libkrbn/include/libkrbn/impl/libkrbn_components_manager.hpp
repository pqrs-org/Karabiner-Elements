#pragma once

#include "libkrbn/impl/libkrbn_complex_modifications_assets_manager.hpp"
#include "libkrbn/impl/libkrbn_configuration_monitor.hpp"
#include "libkrbn/impl/libkrbn_console_user_server_client.hpp"
#include "libkrbn/impl/libkrbn_core_service_daemon_client.hpp"
#include "libkrbn/impl/libkrbn_dispatcher_client.hpp"
#include "libkrbn/impl/libkrbn_file_monitors.hpp"
#include "libkrbn/impl/libkrbn_hid_value_monitor.hpp"
#include "libkrbn/impl/libkrbn_log_monitor.hpp"
#include "libkrbn/impl/libkrbn_process_codesign_monitor.hpp"
#include "libkrbn/impl/libkrbn_version_monitor.hpp"
#include <pqrs/gsl.hpp>

class libkrbn_components_manager {
public:
  libkrbn_components_manager()
      : dispatcher_client_(std::make_shared<libkrbn_dispatcher_client>()) {
  }

  void enqueue_callback(void (*callback)()) {
    dispatcher_client_->enqueue(callback);
  }

  //
  // version_monitor_
  //

  void enable_version_monitor() {
    if (!version_monitor_) {
      version_monitor_ = std::make_shared<libkrbn_version_monitor>();
    }
  }

  void disable_version_monitor() {
    version_monitor_ = nullptr;
  }

  [[nodiscard]] std::shared_ptr<libkrbn_version_monitor> get_libkrbn_version_monitor() const {
    return version_monitor_;
  }

  [[nodiscard]] std::string get_current_version() const {
    if (auto m = version_monitor_) {
      return m->get_version();
    }
    return "";
  }

  //
  // process_codesign_monitor_
  //

  void enable_process_codesign_monitor() {
    if (!process_codesign_monitor_) {
      process_codesign_monitor_ = std::make_shared<libkrbn_process_codesign_monitor>();
    }
  }

  void disable_process_codesign_monitor() {
    process_codesign_monitor_ = nullptr;
  }

  [[nodiscard]] std::shared_ptr<libkrbn_process_codesign_monitor> get_libkrbn_process_codesign_monitor() const {
    return process_codesign_monitor_;
  }

  //
  // configuration_monitor_
  //

  void enable_configuration_monitor() {
    if (!configuration_monitor_) {
      configuration_monitor_ = std::make_shared<libkrbn_configuration_monitor>();
    }
  }

  void disable_configuration_monitor() {
    configuration_monitor_ = nullptr;
  }

  [[nodiscard]] std::shared_ptr<libkrbn_configuration_monitor> get_libkrbn_configuration_monitor() const {
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
      complex_modifications_assets_manager_ = std::make_unique<libkrbn_complex_modifications_assets_manager>();
    }
  }

  void disable_complex_modifications_assets_manager() {
    complex_modifications_assets_manager_ = nullptr;
  }

  [[nodiscard]] std::shared_ptr<libkrbn_complex_modifications_assets_manager> get_complex_modifications_assets_manager() const {
    return complex_modifications_assets_manager_;
  }

  //
  // file_monitors_
  //

  void enable_file_monitors() {
    if (!file_monitors_) {
      file_monitors_ = std::make_shared<libkrbn_file_monitors>();
    }
  }

  void disable_file_monitors() {
    file_monitors_ = nullptr;
  }

  [[nodiscard]] std::shared_ptr<libkrbn_file_monitors> get_libkrbn_file_monitors() const {
    return file_monitors_;
  }

  //
  // log_monitor_
  //

  void enable_log_monitor() {
    if (!log_monitor_) {
      log_monitor_ = std::make_shared<libkrbn_log_monitor>();
    }
  }

  void disable_log_monitor() {
    log_monitor_ = nullptr;
  }

  [[nodiscard]] std::shared_ptr<libkrbn_log_monitor> get_libkrbn_log_monitor() const {
    return log_monitor_;
  }

  [[nodiscard]] std::shared_ptr<std::deque<std::string>> get_current_log_lines() const {
    if (auto m = log_monitor_) {
      return m->get_lines();
    }
    return nullptr;
  }

  //
  // hid_value_monitor_
  //

  void enable_hid_value_monitor() {
    if (!hid_value_monitor_) {
      hid_value_monitor_ = std::make_unique<libkrbn_hid_value_monitor>();
    }
  }

  void disable_hid_value_monitor() {
    hid_value_monitor_ = nullptr;
  }

  [[nodiscard]] std::shared_ptr<libkrbn_hid_value_monitor> get_libkrbn_hid_value_monitor() const {
    return hid_value_monitor_;
  }

  //
  // core_service_daemon_client_
  //

  void enable_core_service_daemon_client() {
    if (!core_service_daemon_client_) {
      core_service_daemon_client_ = std::make_shared<libkrbn_core_service_daemon_client>();
    }
  }

  void disable_core_service_daemon_client() {
    core_service_daemon_client_ = nullptr;
  }

  [[nodiscard]] std::shared_ptr<libkrbn_core_service_daemon_client> get_libkrbn_core_service_daemon_client() const {
    return core_service_daemon_client_;
  }

  //
  // console_user_server_client_
  //

  void enable_console_user_server_client(uid_t uid) {
    if (!console_user_server_client_) {
      console_user_server_client_ = std::make_shared<libkrbn_console_user_server_client>(uid);
    }
  }

  void disable_console_user_server_client() {
    console_user_server_client_ = nullptr;
  }

  [[nodiscard]] std::shared_ptr<libkrbn_console_user_server_client> get_libkrbn_console_user_server_client() const {
    return console_user_server_client_;
  }

private:
  pqrs::not_null_shared_ptr_t<libkrbn_dispatcher_client> dispatcher_client_;
  std::shared_ptr<libkrbn_version_monitor> version_monitor_;
  std::shared_ptr<libkrbn_process_codesign_monitor> process_codesign_monitor_;
  std::shared_ptr<libkrbn_configuration_monitor> configuration_monitor_;
  std::shared_ptr<libkrbn_complex_modifications_assets_manager> complex_modifications_assets_manager_;
  std::shared_ptr<libkrbn_file_monitors> file_monitors_;
  std::shared_ptr<libkrbn_log_monitor> log_monitor_;
  std::shared_ptr<libkrbn_hid_value_monitor> hid_value_monitor_;
  std::shared_ptr<libkrbn_core_service_daemon_client> core_service_daemon_client_;
  std::shared_ptr<libkrbn_console_user_server_client> console_user_server_client_;
};
