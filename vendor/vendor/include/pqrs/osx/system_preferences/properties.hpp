#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "system_preferences.hpp"
#include <IOKit/IOKitLib.h>
#include <map>
#include <pqrs/cf/string.hpp>
#include <pqrs/hash.hpp>
#include <pqrs/hid.hpp>
#include <pqrs/osx/iokit_object_ptr.hpp>

namespace pqrs::osx::system_preferences {
class properties final {
public:
  properties() noexcept = default;

  [[nodiscard]] bool get_use_fkeys_as_standard_function_keys() const noexcept {
    return use_fkeys_as_standard_function_keys_;
  }

  void set_use_fkeys_as_standard_function_keys(bool value) noexcept {
    use_fkeys_as_standard_function_keys_ = value;
  }

  [[nodiscard]] bool get_scroll_direction_is_natural() const noexcept {
    return scroll_direction_is_natural_;
  }

  void set_scroll_direction_is_natural(bool value) noexcept {
    scroll_direction_is_natural_ = value;
  }

  void update() {
    //
    // use_fkeys_as_standard_function_keys_
    //

    // The setting is saved in `com.apple.keyboard.fnState` under `Apple Global Domain`.
    // However, retrieving data using CFPreferencesCopyAppValue can result in issues,
    // such as returning no value for guest users.
    //
    // Therefore, instead of retrieving the setting from CFPreferences,
    // we'll get it from the corresponding parameter in IOHIDSystem, which stays in sync with that setting.

    if (auto entry = pqrs::osx::adopt_iokit_object_ptr(IORegistryEntryFromPath(kIOMainPortDefault,
                                                                               "IOService:/IOResources/IOHIDSystem"))) {
      if (auto property = pqrs::cf::adopt_cf_ptr(IORegistryEntryCreateCFProperty(*entry,
                                                                                 CFSTR("HIDParameters"),
                                                                                 kCFAllocatorDefault,
                                                                                 0))) {
        if (CFDictionaryGetTypeID() == CFGetTypeID(*property)) {
          auto dict = static_cast<CFDictionaryRef>(*property);
          if (auto value = pqrs::cf::make_number<int32_t>(CFDictionaryGetValue(dict, CFSTR("HIDFKeyMode")))) {
            use_fkeys_as_standard_function_keys_ = *value;
          }
        }
      }
    }

    //
    // scroll_direction_is_natural_
    //

    // Since this setting isn't mirrored in IOHIDSystem, we'll retrieve it from CFPreferences instead.
    // Because it's a per-user setting, it can be accessed from CFPreferences without issues.

    if (auto value = find_app_bool_property(CFSTR("com.apple.swipescrolldirection"),
                                            CFSTR("Apple Global Domain"))) {
      scroll_direction_is_natural_ = *value;
    }
  }

  [[nodiscard]] bool operator==(const properties& other) const noexcept = default;

private:
  bool use_fkeys_as_standard_function_keys_ = false;
  bool scroll_direction_is_natural_ = true;
};
} // namespace pqrs::osx::system_preferences

namespace std {
template <>
struct hash<pqrs::osx::system_preferences::properties> final {
  [[nodiscard]] std::size_t operator()(const pqrs::osx::system_preferences::properties& value) const noexcept {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_use_fkeys_as_standard_function_keys());
    pqrs::hash::combine(h, value.get_scroll_direction_is_natural());

    return h;
  }
};
} // namespace std
