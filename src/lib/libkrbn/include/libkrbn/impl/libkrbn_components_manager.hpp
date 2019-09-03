#pragma once

#include "libkrbn/impl/libkrbn_complex_modifications_assets_manager.hpp"
#include "libkrbn/impl/libkrbn_configuration_monitor.hpp"
#include "libkrbn/impl/libkrbn_connected_devices_monitor.hpp"
#include "libkrbn/impl/libkrbn_file_monitor.hpp"
#include "libkrbn/impl/libkrbn_frontmost_application_monitor.hpp"
#include "libkrbn/impl/libkrbn_grabber_client.hpp"
#include "libkrbn/impl/libkrbn_hid_value_monitor.hpp"
#include "libkrbn/impl/libkrbn_log_monitor.hpp"
#include "libkrbn/impl/libkrbn_system_preferences_monitor.hpp"
#include "libkrbn/impl/libkrbn_version_monitor.hpp"

class libkrbn_components_manager {
public:
  //
  // version_monitor_
  //

  void enable_version_monitor(libkrbn_version_monitor_callback callback,
                              void* refcon) {
    version_monitor_ = std::make_unique<libkrbn_version_monitor>(callback,
                                                                 refcon);
  }

  void disable_version_monitor(void) {
    version_monitor_ = nullptr;
  }

  //
  // configuration_monitor_
  //

  void enable_configuration_monitor(libkrbn_configuration_monitor_callback callback,
                                    void* refcon) {
    configuration_monitor_ = std::make_unique<libkrbn_configuration_monitor>(callback,
                                                                             refcon);
  }

  void disable_configuration_monitor(void) {
    configuration_monitor_ = nullptr;
  }

  //
  // complex_modifications_assets_manager_;
  //

  void enable_complex_modifications_assets_manager(void) {
    complex_modifications_assets_manager_ = std::make_unique<libkrbn_complex_modifications_assets_manager>();
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

  void enable_system_preferences_monitor(libkrbn_system_preferences_monitor_callback callback,
                                         void* refcon) {
    system_preferences_monitor_ = std::make_unique<libkrbn_system_preferences_monitor>(callback,
                                                                                       refcon);
  }

  void disable_system_preferences_monitor(void) {
    system_preferences_monitor_ = nullptr;
  }

  //
  // connected_devices_monitor_
  //

  void enable_connected_devices_monitor(libkrbn_connected_devices_monitor_callback callback,
                                        void* refcon) {
    connected_devices_monitor_ = std::make_unique<libkrbn_connected_devices_monitor>(callback,
                                                                                     refcon);
  }

  void disable_connected_devices_monitor(void) {
    connected_devices_monitor_ = nullptr;
  }

  //
  // kextd_state_json_file_monitor
  //

  void enable_kextd_state_json_file_monitor(libkrbn_file_monitor_callback callback,
                                            void* refcon) {
    kextd_state_json_file_monitor_ = std::make_unique<libkrbn_file_monitor>(krbn::constants::get_kextd_state_json_file_path(),
                                                                            callback,
                                                                            refcon);
  }

  void disable_kextd_state_json_file_monitor(void) {
    kextd_state_json_file_monitor_ = nullptr;
  }

  //
  // observer_state_json_file_monitor
  //

  void enable_observer_state_json_file_monitor(libkrbn_file_monitor_callback callback,
                                               void* refcon) {
    observer_state_json_file_monitor_ = std::make_unique<libkrbn_file_monitor>(krbn::constants::get_observer_state_json_file_path(),
                                                                               callback,
                                                                               refcon);
  }

  void disable_observer_state_json_file_monitor(void) {
    observer_state_json_file_monitor_ = nullptr;
  }

  //
  // grabber_state_json_file_monitor
  //

  void enable_grabber_state_json_file_monitor(libkrbn_file_monitor_callback callback,
                                              void* refcon) {
    grabber_state_json_file_monitor_ = std::make_unique<libkrbn_file_monitor>(krbn::constants::get_grabber_state_json_file_path(),
                                                                              callback,
                                                                              refcon);
  }

  void disable_grabber_state_json_file_monitor(void) {
    grabber_state_json_file_monitor_ = nullptr;
  }

  //
  // device_details_json_file_monitor
  //

  void enable_device_details_json_file_monitor(libkrbn_file_monitor_callback callback,
                                               void* refcon) {
    device_details_json_file_monitor_ = std::make_unique<libkrbn_file_monitor>(krbn::constants::get_device_details_json_file_path(),
                                                                               callback,
                                                                               refcon);
  }

  void disable_device_details_json_file_monitor(void) {
    device_details_json_file_monitor_ = nullptr;
  }

  //
  // manipulator_environment_json_file_monitor
  //

  void enable_manipulator_environment_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                        void* refcon) {
    manipulator_environment_json_file_monitor_ = std::make_unique<libkrbn_file_monitor>(krbn::constants::get_manipulator_environment_json_file_path(),
                                                                                        callback,
                                                                                        refcon);
  }

  void disable_manipulator_environment_json_file_monitor(void) {
    manipulator_environment_json_file_monitor_ = nullptr;
  }

  //
  // notification_message_json_file_monitor
  //

  void enable_notification_message_json_file_monitor(libkrbn_file_monitor_callback callback,
                                                     void* refcon) {
    notification_message_json_file_monitor_ = std::make_unique<libkrbn_file_monitor>(krbn::constants::get_user_notification_message_file_path(),
                                                                                     callback,
                                                                                     refcon);
  }

  void disable_notification_message_json_file_monitor(void) {
    notification_message_json_file_monitor_ = nullptr;
  }

  //
  // frontmost_application_monitor_
  //

  void enable_frontmost_application_monitor(libkrbn_frontmost_application_monitor_callback callback,
                                            void* refcon) {
    frontmost_application_monitor_ = std::make_unique<libkrbn_frontmost_application_monitor>(callback,
                                                                                             refcon);
  }

  void disable_frontmost_application_monitor(void) {
    frontmost_application_monitor_ = nullptr;
  }

  //
  // log_monitor_
  //

  void enable_log_monitor(libkrbn_log_monitor_callback callback,
                          void* refcon) {
    log_monitor_ = std::make_unique<libkrbn_log_monitor>(callback,
                                                         refcon);
  }

  void disable_log_monitor(void) {
    log_monitor_ = nullptr;
  }

  //
  // hid_value_monitor_
  //

  void enable_hid_value_monitor(libkrbn_hid_value_monitor_callback callback,
                                void* refcon) {
    hid_value_monitor_ = std::make_unique<libkrbn_hid_value_monitor>(callback,
                                                                     refcon);
  }

  void disable_hid_value_monitor(void) {
    hid_value_monitor_ = nullptr;
  }

  bool hid_value_monitor_observed(void) {
    if (hid_value_monitor_) {
      return hid_value_monitor_->get_observed();
    }
    return false;
  }

  //
  // grabber_client_
  //

  void enable_grabber_client(libkrbn_grabber_client_connected_callback connected_callback,
                             libkrbn_grabber_client_connect_failed_callback connect_failed_callback,
                             libkrbn_grabber_client_closed_callback closed_callback) {
    grabber_client_ = std::make_unique<libkrbn_grabber_client>(connected_callback,
                                                               connect_failed_callback,
                                                               closed_callback);
  }

  void disable_grabber_client(void) {
    grabber_client_ = nullptr;
  }

  void grabber_client_async_set_variable(const std::string& name, int value) {
    grabber_client_->async_set_variable(name, value);
  }

  void grabber_client_sync_set_variable(const std::string& name, int value) {
    grabber_client_->sync_set_variable(name, value);
  }

private:
  std::unique_ptr<libkrbn_version_monitor> version_monitor_;
  std::unique_ptr<libkrbn_configuration_monitor> configuration_monitor_;
  std::shared_ptr<libkrbn_complex_modifications_assets_manager> complex_modifications_assets_manager_;
  std::unique_ptr<libkrbn_system_preferences_monitor> system_preferences_monitor_;
  std::unique_ptr<libkrbn_connected_devices_monitor> connected_devices_monitor_;
  std::unique_ptr<libkrbn_file_monitor> kextd_state_json_file_monitor_;
  std::unique_ptr<libkrbn_file_monitor> observer_state_json_file_monitor_;
  std::unique_ptr<libkrbn_file_monitor> grabber_state_json_file_monitor_;
  std::unique_ptr<libkrbn_file_monitor> device_details_json_file_monitor_;
  std::unique_ptr<libkrbn_file_monitor> manipulator_environment_json_file_monitor_;
  std::unique_ptr<libkrbn_file_monitor> notification_message_json_file_monitor_;
  std::unique_ptr<libkrbn_frontmost_application_monitor> frontmost_application_monitor_;
  std::unique_ptr<libkrbn_log_monitor> log_monitor_;
  std::unique_ptr<libkrbn_hid_value_monitor> hid_value_monitor_;
  std::unique_ptr<libkrbn_grabber_client> grabber_client_;
};
