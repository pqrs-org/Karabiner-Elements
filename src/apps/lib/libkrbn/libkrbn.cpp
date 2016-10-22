#include "libkrbn.h"
#include "constants.hpp"
#include "libkrbn.hpp"
#include "thread_utility.hpp"
#include "types.hpp"
#include <iostream>

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

const char* libkrbn_get_configuration_core_file_path(void) {
  return constants::get_configuration_core_file_path();
}

bool libkrbn_get_hid_system_key(uint8_t* _Nonnull key, const char* key_name) {
  if (auto key_code = krbn::types::get_key_code(key_name)) {
    if (auto value = krbn::types::get_hid_system_key(*key_code)) {
      *key = *value;
      return true;
    }
  }
  return false;
}
