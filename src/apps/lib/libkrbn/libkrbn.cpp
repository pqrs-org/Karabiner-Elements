#include "libkrbn.h"
#include "constants.hpp"
#include "libkrbn.hpp"
#include <iostream>

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
