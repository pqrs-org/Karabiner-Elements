#pragma once

#include "boost_defs.hpp"

#include <ApplicationServices/ApplicationServices.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <optional>

namespace krbn {
class session final {
public:
  static std::optional<uid_t> get_current_console_user_id(void) {
    uid_t uid;
    if (auto user_name = SCDynamicStoreCopyConsoleUser(nullptr, &uid, nullptr)) {
      CFRelease(user_name);
      return uid;
    }

    return std::nullopt;
  }

  static std::optional<bool> is_active(void) {
    std::optional<bool> result;

    if (CFDictionaryRef dictionary = CGSessionCopyCurrentDictionary()) {
      if (CFBooleanRef b = static_cast<CFBooleanRef>(CFDictionaryGetValue(dictionary, kCGSessionOnConsoleKey))) {
        result = CFBooleanGetValue(b);
      }

      CFRelease(dictionary);
    }

    return result;
  }
};
} // namespace krbn
