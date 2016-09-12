#include "libkrbn.h"
#include "constants.hpp"
#include <thread>
#include <vector>

class libkrbn {
public:
  static bool cfstring_to_cstring(std::vector<char>& v, CFStringRef string) {
    if (string) {
      if (auto cstring = CFStringGetCStringPtr(string, kCFStringEncodingUTF8)) {
        auto length = strlen(cstring) + 1;
        v.resize(length);
        strlcpy(&(v[0]), cstring, length);
        return true;
      }
    }

    v.resize(1);
    v[0] = '\0';
    return false;
  }
};

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

const char* libkrbn_get_distributed_notification_console_user_socket_directory_is_ready(void) {
  static std::mutex mutex;
  static bool once = false;
  static std::vector<char> result;

  std::lock_guard<std::mutex> guard(mutex);

  if (!once) {
    once = true;
    libkrbn::cfstring_to_cstring(result, constants::get_distributed_notification_console_user_socket_directory_is_ready());
  }

  return &(result[0]);
}
