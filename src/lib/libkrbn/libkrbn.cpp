#include "libkrbn.h"
#include "constants.hpp"
#include "core_configuration.hpp"
#include "libkrbn.hpp"
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
  static std::mutex mutex;
  static bool once = false;
  static std::vector<char> result;

  std::lock_guard<std::mutex> guard(mutex);

  if (!once) {
    once = true;
    libkrbn::cfstring_to_cstring(result, constants::get_distributed_notification_observed_object());
  }

  return &(result[0]);
}

const char* libkrbn_get_distributed_notification_grabber_is_launched(void) {
  static std::mutex mutex;
  static bool once = false;
  static std::vector<char> result;

  std::lock_guard<std::mutex> guard(mutex);

  if (!once) {
    once = true;
    libkrbn::cfstring_to_cstring(result, constants::get_distributed_notification_grabber_is_launched());
  }

  return &(result[0]);
}

const char* libkrbn_get_core_configuration_file_path(void) {
  return constants::get_core_configuration_file_path();
}

const char* libkrbn_get_devices_json_file_path(void) {
  return constants::get_devices_json_file_path();
}

const char* _Nonnull libkrbn_get_default_profile_json_string(void) {
  static std::mutex mutex;
  static bool once = false;
  static std::string result;

  std::lock_guard<std::mutex> guard(mutex);

  if (!once) {
    once = true;

    result = core_configuration::get_default_profile().dump();
  }

  return result.c_str();
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

void libkrbn_check_for_updates_in_background(void) {
  update_utility::check_for_updates_in_background();
}

void libkrbn_check_for_updates_stable_only(void) {
  update_utility::check_for_updates_stable_only();
}

void libkrbn_check_for_updates_with_beta_version(void) {
  update_utility::check_for_updates_with_beta_version();
}
