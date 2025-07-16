#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/cf/number.hpp>

namespace pqrs {
namespace osx {
namespace system_preferences {
enum class user {
  current_user,
  any_user,
};
enum class host {
  current_host,
  any_host,
};

inline std::optional<bool> find_app_bool_property(CFStringRef key,
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

inline std::optional<float> find_app_float_property(CFStringRef key,
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

inline cf::cf_ptr<CFDictionaryRef> find_app_dictionary_property(CFStringRef key,
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

inline CFStringRef get_user_name(user user) {
  switch (user) {
    case user::current_user:
      return kCFPreferencesCurrentUser;
    case user::any_user:
      return kCFPreferencesAnyUser;
  }
}

inline CFStringRef get_host_name(host host) {
  switch (host) {
    case host::current_host:
      return kCFPreferencesCurrentHost;
    case host::any_host:
      return kCFPreferencesAnyHost;
  }
}

inline std::optional<bool> find_bool_property(CFStringRef key,
                                              CFStringRef application_id,
                                              user user,
                                              host host) {
  std::optional<bool> value = std::nullopt;

  if (auto v = CFPreferencesCopyValue(key,
                                      application_id,
                                      get_user_name(user),
                                      get_host_name(host))) {
    if (CFBooleanGetTypeID() == CFGetTypeID(v)) {
      value = CFBooleanGetValue(static_cast<CFBooleanRef>(v));
    }
    CFRelease(v);
  }

  return value;
}

inline std::optional<float> find_float_property(CFStringRef key,
                                                CFStringRef application_id,
                                                user user,
                                                host host) {
  std::optional<float> value = std::nullopt;

  if (auto v = CFPreferencesCopyValue(key,
                                      application_id,
                                      get_user_name(user),
                                      get_host_name(host))) {
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
                                                            CFStringRef application_id,
                                                            user user,
                                                            host host) {
  cf::cf_ptr<CFDictionaryRef> result;

  if (auto v = CFPreferencesCopyValue(key,
                                      application_id,
                                      get_user_name(user),
                                      get_host_name(host))) {
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
