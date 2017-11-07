#include "libkrbn.h"
#include "application_launcher.hpp"
#include "constants.hpp"
#include "core_configuration.hpp"
#include "launchctl_utility.hpp"
#include "libkrbn_cpp.hpp"
#include "process_utility.hpp"
#include "thread_utility.hpp"
#include "types.hpp"
#include "update_utility.hpp"
#include <fstream>
#include <iostream>
#include <json/json.hpp>
#include <string>

void libkrbn_initialize(void) {
  krbn::thread_utility::register_main_thread();
}

const char* libkrbn_get_distributed_notification_observed_object(void) {
  return krbn::constants::get_distributed_notification_observed_object();
}

const char* libkrbn_get_distributed_notification_grabber_is_launched(void) {
  return krbn::constants::get_distributed_notification_grabber_is_launched();
}

const char* libkrbn_get_distributed_notification_console_user_server_is_disabled(void) {
  return krbn::constants::get_distributed_notification_console_user_server_is_disabled();
}

const char* libkrbn_get_grabber_alerts_json_file_path(void) {
  return krbn::constants::get_grabber_alerts_json_file_path();
}

const char* libkrbn_get_devices_json_file_path(void) {
  return krbn::constants::get_devices_json_file_path();
}

const char* libkrbn_get_manipulator_environment_json_file_path(void) {
  return krbn::constants::get_manipulator_environment_json_file_path();
}

const char* libkrbn_get_user_complex_modifications_assets_directory(void) {
  return krbn::constants::get_user_complex_modifications_assets_directory().c_str();
}

bool libkrbn_lock_single_application_with_user_pid_file(const char* _Nonnull pid_file_name) {
  return krbn::process_utility::lock_single_application_with_user_pid_file(pid_file_name);
}

void libkrbn_unlock_single_application(void) {
  krbn::process_utility::unlock_single_application();
}

bool libkrbn_save_beautified_json_string(const char* _Nonnull file_path, const char* _Nonnull json_string) {
  try {
    // nlohmann::json sorts dictionary keys.
    std::ofstream stream(file_path);
    if (stream) {
      auto json = nlohmann::json::parse(json_string);
      stream << std::setw(4) << json << std::endl;
      return true;
    }
  } catch (std::exception& e) {
  }
  return false;
}

void libkrbn_launchctl_manage_console_user_server(bool load) {
  krbn::launchctl_utility::manage_console_user_server(load);
}

void libkrbn_launchctl_restart_console_user_server(void) {
  krbn::launchctl_utility::restart_console_user_server();
}

void libkrbn_check_for_updates_in_background(void) {
  krbn::update_utility::check_for_updates_in_background();
}

void libkrbn_check_for_updates_stable_only(void) {
  krbn::update_utility::check_for_updates_stable_only();
}

void libkrbn_check_for_updates_with_beta_version(void) {
  krbn::update_utility::check_for_updates_with_beta_version();
}

void libkrbn_launch_event_viewer(void) {
  krbn::application_launcher::launch_event_viewer();
}

void libkrbn_launch_menu(void) {
  krbn::application_launcher::launch_menu();
}

void libkrbn_launch_preferences(void) {
  krbn::application_launcher::launch_preferences();
}

bool libkrbn_system_core_configuration_file_path_exists(void) {
  return krbn::filesystem::exists(krbn::constants::get_system_core_configuration_file_path());
}

bool libkrbn_device_identifiers_is_apple(const libkrbn_device_identifiers* p) {
  if (p) {
    return libkrbn_cpp::make_device_identifiers(*p).is_apple();
  }
  return false;
}
