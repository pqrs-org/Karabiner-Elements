#pragma once

#include "types.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <optional>
#include <pqrs/cf/number.hpp>

namespace krbn {
class system_preferences_utility final {
public:
  static std::optional<bool> get_bool_property(CFStringRef _Nonnull key, CFStringRef _Nonnull application_id) {
    std::optional<bool> value = std::nullopt;
    if (auto v = CFPreferencesCopyAppValue(key, application_id)) {
      if (CFBooleanGetTypeID() == CFGetTypeID(v)) {
        value = CFBooleanGetValue(static_cast<CFBooleanRef>(v));
      }
      CFRelease(v);
    }
    return value;
  }

  static std::optional<float> get_float_property(CFStringRef _Nonnull key, CFStringRef _Nonnull application_id) {
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

  static CFDictionaryRef _Nullable copy_dictionary_property(CFStringRef _Nonnull key, CFStringRef _Nonnull application_id) {
    CFDictionaryRef value = nullptr;
    if (auto v = CFPreferencesCopyAppValue(key, application_id)) {
      if (CFDictionaryGetTypeID() == CFGetTypeID(v)) {
        CFRetain(v);
        value = static_cast<CFDictionaryRef>(v);
      }
      CFRelease(v);
    }
    return value;
  }

  /// "Use all F1, F2, etc. keys as standard function keys."
  static bool get_keyboard_fn_state(void) {
    if (auto value = get_bool_property(CFSTR("com.apple.keyboard.fnState"), CFSTR("Apple Global Domain"))) {
      return *value;
    }
    // default value
    return false;
  }

  // Scroll direction (Natural == 1)
  static bool get_swipe_scroll_direction(void) {
    if (auto value = get_bool_property(CFSTR("com.apple.swipescrolldirection"), CFSTR("Apple Global Domain"))) {
      return *value;
    }
    // default value
    return true;
  }

  static uint8_t get_keyboard_type(vendor_id vendor_id, product_id product_id, uint8_t country_code) {
    if (auto value = copy_dictionary_property(CFSTR("keyboardtype"), CFSTR("com.apple.keyboardtype"))) {
      if (auto key = CFStringCreateWithCString(kCFAllocatorDefault,
                                               fmt::format("{0}-{1}-{2}",
                                                           type_safe::get(product_id),
                                                           type_safe::get(vendor_id),
                                                           country_code)
                                                   .c_str(),
                                               kCFStringEncodingUTF8)) {
        if (auto keyboard_type = CFDictionaryGetValue(value, key)) {
          if (auto v = pqrs::cf::make_number<int64_t>(keyboard_type)) {
            return static_cast<uint8_t>(*v);
          }
        }

        CFRelease(key);
      }

      CFRelease(value);
    }

    // default value
    return 40;
  }

  static std::string get_keyboard_type_string(uint8_t keyboard_type) {
    using namespace std::string_literals;

    switch (keyboard_type) {
      case 41:
        return "iso"s;
      case 42:
        return "jis"s;
    }
    return "ansi"s;
  }
};
} // namespace krbn
