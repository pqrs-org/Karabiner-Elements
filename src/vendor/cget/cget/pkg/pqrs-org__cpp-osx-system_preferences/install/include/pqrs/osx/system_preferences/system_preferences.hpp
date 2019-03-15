#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "keyboard_type_key.hpp"
#include <pqrs/cf/number.hpp>

namespace pqrs {
namespace osx {
namespace system_preferences {
inline std::optional<bool> find_bool_property(CFStringRef key,
                                              CFStringRef application_id) {
  std::optional<bool> value = std::nullopt;

  if (auto v = CFPreferencesCopyAppValue(key, application_id)) {
    if (CFBooleanGetTypeID() == CFGetTypeID(v)) {
      value = CFBooleanGetValue(static_cast<CFBooleanRef>(v));
    }
    CFRelease(v);
  }

  return value;
}

inline std::optional<float> find_float_property(CFStringRef key,
                                                CFStringRef application_id) {
  std::optional<float> value = std::nullopt;

  if (auto v = CFPreferencesCopyAppValue(key, application_id)) {
    if (CFNumberGetTypeID() == CFGetTypeID(v)) {
      float vv;
      if (CFNumberGetValue(static_cast<CFNumberRef>(v), kCFNumberFloatType, &vv)) {
        value = vv;
      }
    }
    CFRelease(v);
  }

  return value;
}

inline cf::cf_ptr<CFDictionaryRef> find_dictionary_property(CFStringRef key,
                                                            CFStringRef application_id) {
  cf::cf_ptr<CFDictionaryRef> result;

  if (auto v = CFPreferencesCopyAppValue(key, application_id)) {
    if (CFDictionaryGetTypeID() == CFGetTypeID(v)) {
      result = static_cast<CFDictionaryRef>(v);
    }
    CFRelease(v);
  }

  return result;
}
} // namespace system_preferences
} // namespace osx
} // namespace pqrs
