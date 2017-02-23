#include "libkrbn.h"
#include "application_launcher.hpp"
#include "constants.hpp"
#include "core_configuration.hpp"
#include "launchctl_utility.hpp"
#include "libkrbn.hpp"
#include "process_utility.hpp"
#include "thread_utility.hpp"
#include "types.hpp"
#include "update_utility.hpp"
#include <fstream>
#include <iostream>
#include <json/json.hpp>
#include <string>

void libkrbn_initialize(void) {
  thread_utility::register_main_thread();
}

const char* libkrbn_get_distributed_notification_observed_object(void) {
  return constants::get_distributed_notification_observed_object();
}

const char* libkrbn_get_distributed_notification_grabber_is_launched(void) {
  return constants::get_distributed_notification_grabber_is_launched();
}

const char* libkrbn_get_distributed_notification_console_user_server_is_disabled(void) {
  return constants::get_distributed_notification_console_user_server_is_disabled();
}

const char* libkrbn_get_core_configuration_file_path(void) {
  return constants::get_core_configuration_file_path().c_str();
}

const char* libkrbn_get_devices_json_file_path(void) {
  return constants::get_devices_json_file_path();
}

bool libkrbn_lock_single_application_with_user_pid_file(const char* _Nonnull pid_file_name) {
  return process_utility::lock_single_application_with_user_pid_file(pid_file_name);
}

void libkrbn_unlock_single_application(void) {
  process_utility::unlock_single_application();
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
  launchctl_utility::manage_console_user_server(load);
}

void libkrbn_check_for_updates_in_background(void) {
  update_utility::check_for_updates_in_background();
}

void libkrbn_check_for_updates_stable_only(void) {
  update_utility::check_for_updates_stable_only();
}

void libkrbn_check_for_updates_with_beta_version(void) {
  update_utility::check_for_updates_with_beta_version();
}

void libkrbn_launch_event_viewer(void) {
  application_launcher::launch_event_viewer();
}

void libkrbn_launch_menu(void) {
  application_launcher::launch_menu();
}

void libkrbn_launch_preferences(void) {
  application_launcher::launch_preferences();
}
