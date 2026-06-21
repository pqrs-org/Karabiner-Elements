#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <pqrs/cf/number.hpp>
#include <utility>

namespace pqrs::osx::system_preferences {
enum class user {
  current_user,
  any_user,
};
enum class host {
  current_host,
  any_host,
};

[[nodiscard]] inline std::optional<bool> find_app_bool_property(CFStringRef key,
                                                                CFStringRef application_id) {
  std::optional<bool> value = std::nullopt;

  if (auto v = cf::adopt_cf_ptr(CFPreferencesCopyAppValue(key, application_id))) {
    if (CFBooleanGetTypeID() == CFGetTypeID(*v)) {
      value = CFBooleanGetValue(static_cast<CFBooleanRef>(*v));
    }
  }

  return value;
}

[[nodiscard]] inline std::optional<float> find_app_float_property(CFStringRef key,
                                                                  CFStringRef application_id) {
  std::optional<float> value = std::nullopt;

  if (auto v = cf::adopt_cf_ptr(CFPreferencesCopyAppValue(key, application_id))) {
    if (CFNumberGetTypeID() == CFGetTypeID(*v)) {
      float vv;
      if (CFNumberGetValue(static_cast<CFNumberRef>(*v), kCFNumberFloatType, &vv)) {
        value = vv;
      }
    }
  }

  return value;
}

[[nodiscard]] inline cf::cf_ptr<CFDictionaryRef> find_app_dictionary_property(CFStringRef key,
                                                                              CFStringRef application_id) {
  cf::cf_ptr<CFDictionaryRef> result;

  if (auto v = cf::adopt_cf_ptr(CFPreferencesCopyAppValue(key, application_id))) {
    if (CFDictionaryGetTypeID() == CFGetTypeID(*v)) {
      result = static_cast<CFDictionaryRef>(*v);
    }
  }

  return result;
}

[[nodiscard]] inline CFStringRef get_user_name(user user) noexcept {
  switch (user) {
    case user::current_user:
      return kCFPreferencesCurrentUser;
    case user::any_user:
      return kCFPreferencesAnyUser;
  }

  std::unreachable();
}

[[nodiscard]] inline CFStringRef get_host_name(host host) noexcept {
  switch (host) {
    case host::current_host:
      return kCFPreferencesCurrentHost;
    case host::any_host:
      return kCFPreferencesAnyHost;
  }

  std::unreachable();
}

[[nodiscard]] inline std::optional<bool> find_bool_property(CFStringRef key,
                                                            CFStringRef application_id,
                                                            user user,
                                                            host host) {
  std::optional<bool> value = std::nullopt;

  if (auto v = cf::adopt_cf_ptr(CFPreferencesCopyValue(key,
                                                       application_id,
                                                       get_user_name(user),
                                                       get_host_name(host)))) {
    if (CFBooleanGetTypeID() == CFGetTypeID(*v)) {
      value = CFBooleanGetValue(static_cast<CFBooleanRef>(*v));
    }
  }

  return value;
}

[[nodiscard]] inline std::optional<float> find_float_property(CFStringRef key,
                                                              CFStringRef application_id,
                                                              user user,
                                                              host host) {
  std::optional<float> value = std::nullopt;

  if (auto v = cf::adopt_cf_ptr(CFPreferencesCopyValue(key,
                                                       application_id,
                                                       get_user_name(user),
                                                       get_host_name(host)))) {
    if (CFNumberGetTypeID() == CFGetTypeID(*v)) {
      float vv;
      if (CFNumberGetValue(static_cast<CFNumberRef>(*v), kCFNumberFloatType, &vv)) {
        value = vv;
      }
    }
  }

  return value;
}

[[nodiscard]] inline cf::cf_ptr<CFDictionaryRef> find_dictionary_property(CFStringRef key,
                                                                          CFStringRef application_id,
                                                                          user user,
                                                                          host host) {
  cf::cf_ptr<CFDictionaryRef> result;

  if (auto v = cf::adopt_cf_ptr(CFPreferencesCopyValue(key,
                                                       application_id,
                                                       get_user_name(user),
                                                       get_host_name(host)))) {
    if (CFDictionaryGetTypeID() == CFGetTypeID(*v)) {
      result = static_cast<CFDictionaryRef>(*v);
    }
  }

  return result;
}
} // namespace pqrs::osx::system_preferences
