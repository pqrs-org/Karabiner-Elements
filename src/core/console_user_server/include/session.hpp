#pragma once

#include <ApplicationServices/ApplicationServices.h>

class session final {
public:
  enum class state {
    none,
    active,
    inactive,
  };

  static state get_state(void) {
    state result = state::none;

    if (CFDictionaryRef dictionary = CGSessionCopyCurrentDictionary()) {
      if (CFBooleanRef b = static_cast<CFBooleanRef>(CFDictionaryGetValue(dictionary, kCGSessionOnConsoleKey))) {
        result = CFBooleanGetValue(b) ? state::active : state::inactive;
      }

      CFRelease(dictionary);
    }

    return result;
  }
};
