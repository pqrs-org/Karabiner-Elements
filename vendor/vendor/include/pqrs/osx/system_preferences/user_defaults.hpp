#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "keyboard_type_key.hpp"
#include <pqrs/cf/dictionary.hpp>
#include <pqrs/cf/number.hpp>
#include <pqrs/cf/string.hpp>

namespace pqrs {
namespace osx {
namespace system_preferences {
namespace user_defaults {
// Note:
// `set_keyboard_type` requires root privileges due to `CFPreferencesSynchronize`.
inline void set_keyboard_type(hid::product_id::value_t product_id,
                              hid::vendor_id::value_t vendor_id,
                              hid::country_code::value_t country_code,
                              iokit_keyboard_type::value_t keyboard_type) {
  auto app_value_key = CFSTR("keyboardtype");
  auto app_value_application_id = CFSTR("com.apple.keyboardtype");

  cf::cf_ptr<CFMutableDictionaryRef> dictionary_ptr;
  if (auto dictionary = find_dictionary_property(app_value_key,
                                                 app_value_application_id,
                                                 user::any_user,
                                                 host::current_host)) {
    dictionary_ptr = cf::make_cf_mutable_dictionary_copy(*dictionary);
  } else {
    dictionary_ptr = cf::make_cf_mutable_dictionary(0);
  }

  std::string key = std::to_string(type_safe::get(product_id)) + "-" +
                    std::to_string(type_safe::get(vendor_id)) + "-" +
                    std::to_string(type_safe::get(country_code));
  if (auto cf_key = cf::make_cf_string(key)) {
    if (auto cf_value = cf::make_cf_number(static_cast<int64_t>(type_safe::get(keyboard_type)))) {
      CFDictionarySetValue(*dictionary_ptr,
                           *cf_key,
                           *cf_value);

      CFPreferencesSetValue(app_value_key,
                            *dictionary_ptr,
                            app_value_application_id,
                            kCFPreferencesAnyUser,
                            kCFPreferencesCurrentHost);
      CFPreferencesSynchronize(app_value_application_id,
                               kCFPreferencesAnyUser,
                               kCFPreferencesCurrentHost);
    }
  }
}
} // namespace user_defaults
} // namespace system_preferences
} // namespace osx
} // namespace pqrs
