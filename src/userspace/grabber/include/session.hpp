#pragma once

#include <SystemConfiguration/SystemConfiguration.h>

#include "logger.hpp"

class session final {
public:
  static bool get_current_console_user_id(uid_t& user_id) {
    if (auto user_name = SCDynamicStoreCopyConsoleUser(nullptr, &user_id, nullptr)) {
      CFRelease(user_name);
      return true;
    }

    user_id = 0;
    return false;
  }
};
