#pragma once

#include "libkrbn/impl/libkrbn_complex_modifications_assets_manager.hpp"
#include "libkrbn/impl/libkrbn_configuration_monitor.hpp"
#include "libkrbn/impl/libkrbn_connected_devices_monitor.hpp"
#include "libkrbn/impl/libkrbn_dispatcher_client.hpp"
#include "libkrbn/impl/libkrbn_file_monitors.hpp"
#include "libkrbn/impl/libkrbn_frontmost_application_monitor.hpp"
#include "libkrbn/impl/libkrbn_grabber_client.hpp"
#include "libkrbn/impl/libkrbn_hid_value_monitor.hpp"
#include "libkrbn/impl/libkrbn_log_monitor.hpp"
#include "libkrbn/impl/libkrbn_system_preferences_monitor.hpp"
#include "libkrbn/impl/libkrbn_version_monitor.hpp"

class libkrbn_components_manager {
public:
  libkrbn_components_manager(void) {
    dispatcher_client_ = std::make_shared<libkrbn_dispatcher_client>();
  }

  void enqueue_callback(void (*callback)(void)) {
    dispatcher_client_->enqueue(callback);
  }

  //
  // version_monitor_
  //

  void enable_version_monitor(void) {
    if (!version_monitor_) {
      version_monitor_ = std::make_shared<libkrbn_version_monitor>();
    }
  }

  void disable_version_monitor(void) {
    version_monitor_ = nullptr;
  }

  std::shared_ptr<libkrbn_version_monitor> get_libkrbn_version_monitor(void) const {
    return version_monitor_;
  }

  std::string get_current_version(void) const {
    if (auto m = version_monitor_) {
      return m->get_version();
    }
    return "";
  }

  //
  // configuration_monitor_
  //

  void enable_configuration_monitor(void) {
    if (!configuration_monitor_) {
      configuration_monitor_ = std::make_shared<libkrbn_configuration_monitor>();
    }
  }

  void disable_configuration_monitor(void) {
    configuration_monitor_ = nullptr;
  }

  std::shared_ptr<libkrbn_configuration_monitor> get_libkrbn_configuration_monitor(void) const {
    return configuration_monitor_;
  }

  std::shared_ptr<krbn::core_configuration::core_configuration> get_current_core_configuration(void) const {
    if (auto m = configuration_monitor_) {
      return m->get_weak_core_configuration().lock();
    }
    return nullptr;
  }

  //
  // complex_modifications_assets_manager_;
  //

  void enable_complex_modifications_assets_manager(void) {
    if (!complex_modifications_assets_manager_) {
      complex_modifications_assets_manager_ = std::make_unique<libkrbn_complex_modifications_assets_manager>();
    }
  }

  void disable_complex_modifications_assets_manager(void) {
    complex_modifications_assets_manager_ = nullptr;
  }

  std::shared_ptr<libkrbn_complex_modifications_assets_manager> get_complex_modifications_assets_manager(void) const {
    return complex_modifications_assets_manager_;
  }

  //
  // system_preferences_monitor_
  //

  void enable_system_preferences_monitor(void) {
    if (!system_preferences_monitor_) {
      system_preferences_monitor_ = std::make_shared<libkrbn_system_preferences_monitor>();
    }
  }

  void disable_system_preferences_monitor(void) {
    system_preferences_monitor_ = nullptr;
  }

  std::shared_ptr<libkrbn_system_preferences_monitor> get_libkrbn_system_preferences_monitor(void) const {
    return system_preferences_monitor_;
  }

  std::shared_ptr<pqrs::osx::system_preferences::properties> get_current_system_preferences_properties(void) const {
    if (auto m = system_preferences_monitor_) {
      return m->get_weak_system_preferences_properties().lock();
    }
    return nullptr;
  }

  //
  // connected_devices_monitor_
  //

  void enable_connected_devices_monitor(void) {
    if (!connected_devices_monitor_) {
      connected_devices_monitor_ = std::make_shared<libkrbn_connected_devices_monitor>();
    }
  }

  void disable_connected_devices_monitor(void) {
    connected_devices_monitor_ = nullptr;
  }

  std::shared_ptr<libkrbn_connected_devices_monitor> get_libkrbn_connected_devices_monitor(void) const {
    return connected_devices_monitor_;
  }

  std::shared_ptr<const krbn::connected_devices::connected_devices> get_current_connected_devices(void) const {
    if (auto m = connected_devices_monitor_) {
      return m->get_weak_connected_devices().lock();
    }
    return nullptr;
  }

  //
  // file_monitors_
  //

  void enable_file_monitors(void) {
    if (!file_monitors_) {
      file_monitors_ = std::make_shared<libkrbn_file_monitors>();
    }
  }

  void disable_file_monitors(void) {
    file_monitors_ = nullptr;
  }

  std::shared_ptr<libkrbn_file_monitors> get_libkrbn_file_monitors(void) const {
    return file_monitors_;
  }

  //
  // frontmost_application_monitor_
  //

  void enable_frontmost_application_monitor(void) {
    if (!frontmost_application_monitor_) {
      frontmost_application_monitor_ = std::make_unique<libkrbn_frontmost_application_monitor>();
    }
  }

  void disable_frontmost_application_monitor(void) {
    frontmost_application_monitor_ = nullptr;
  }

  std::shared_ptr<libkrbn_frontmost_application_monitor> get_libkrbn_frontmost_application_monitor(void) const {
    return frontmost_application_monitor_;
  }

  //
  // log_monitor_
  //

  void enable_log_monitor(void) {
    if (!log_monitor_) {
      log_monitor_ = std::make_shared<libkrbn_log_monitor>();
    }
  }

  void disable_log_monitor(void) {
    log_monitor_ = nullptr;
  }

  std::shared_ptr<libkrbn_log_monitor> get_libkrbn_log_monitor(void) const {
    return log_monitor_;
  }

  std::shared_ptr<std::deque<std::string>> get_current_log_lines(void) const {
    if (auto m = log_monitor_) {
      return m->get_lines();
    }
    return nullptr;
  }

  //
  // hid_value_monitor_
  //

  void enable_hid_value_monitor(void) {
    if (!hid_value_monitor_) {
      hid_value_monitor_ = std::make_unique<libkrbn_hid_value_monitor>();
    }
  }

  void disable_hid_value_monitor(void) {
    hid_value_monitor_ = nullptr;
  }

  std::shared_ptr<libkrbn_hid_value_monitor> get_libkrbn_hid_value_monitor(void) const {
    return hid_value_monitor_;
  }

  //
  // grabber_client_
  //

  void enable_grabber_client(std::optional<std::string> client_socket_directory_name) {
    if (!grabber_client_) {
      grabber_client_ = std::make_shared<libkrbn_grabber_client>(client_socket_directory_name);
    }
  }

  void disable_grabber_client(void) {
    grabber_client_ = nullptr;
  }

  std::shared_ptr<libkrbn_grabber_client> get_libkrbn_grabber_client(void) const {
    return grabber_client_;
  }

private:
  std::shared_ptr<libkrbn_dispatcher_client> dispatcher_client_;
  std::shared_ptr<libkrbn_version_monitor> version_monitor_;
  std::shared_ptr<libkrbn_configuration_monitor> configuration_monitor_;
  std::shared_ptr<libkrbn_complex_modifications_assets_manager> complex_modifications_assets_manager_;
  std::shared_ptr<libkrbn_system_preferences_monitor> system_preferences_monitor_;
  std::shared_ptr<libkrbn_connected_devices_monitor> connected_devices_monitor_;
  std::shared_ptr<libkrbn_file_monitors> file_monitors_;
  std::shared_ptr<libkrbn_frontmost_application_monitor> frontmost_application_monitor_;
  std::shared_ptr<libkrbn_log_monitor> log_monitor_;
  std::shared_ptr<libkrbn_hid_value_monitor> hid_value_monitor_;
  std::shared_ptr<libkrbn_grabber_client> grabber_client_;
};
