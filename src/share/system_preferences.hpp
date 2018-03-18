#pragma once

#include "boost_defs.hpp"

#include "cf_utility.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <boost/optional.hpp>
#include <cmath>

namespace krbn {
class system_preferences final {
public:
  class values {
  public:
    values(void) : keyboard_fn_state_(system_preferences::get_keyboard_fn_state()),
                   swipe_scroll_direction_(system_preferences::get_swipe_scroll_direction()) {
    }

    bool get_keyboard_fn_state(void) const {
      return keyboard_fn_state_;
    }

    bool get_swipe_scroll_direction(void) const {
      return swipe_scroll_direction_;
    }

    boost::optional<uint8_t> get_keyboard_type(void) const {
      return keyboard_type_;
    }

    bool operator==(const system_preferences::values& other) const {
      return keyboard_fn_state_ == other.keyboard_fn_state_ &&
             swipe_scroll_direction_ == other.swipe_scroll_direction_;
    }

    bool operator!=(const system_preferences::values& other) const { return !(*this == other); }

  private:
    bool keyboard_fn_state_;
    bool swipe_scroll_direction_;
    boost::optional<uint8_t> keyboard_type_;
  };

  static boost::optional<bool> get_bool_property(CFStringRef _Nonnull key, CFStringRef _Nonnull application_id) {
    boost::optional<bool> value = boost::none;
    if (auto v = CFPreferencesCopyAppValue(key, application_id)) {
      if (CFBooleanGetTypeID() == CFGetTypeID(v)) {
        value = CFBooleanGetValue(static_cast<CFBooleanRef>(v));
      }
      CFRelease(v);
    }
    return value;
  }

  static boost::optional<float> get_float_property(CFStringRef _Nonnull key, CFStringRef _Nonnull application_id) {
    boost::optional<float> value = boost::none;
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
    if (auto value = system_preferences::get_bool_property(CFSTR("com.apple.keyboard.fnState"), CFSTR("Apple Global Domain"))) {
      return *value;
    }
    // default value
    return false;
  }

  // Scroll direction (Natural == 1)
  static bool get_swipe_scroll_direction(void) {
    if (auto value = system_preferences::get_bool_property(CFSTR("com.apple.swipescrolldirection"), CFSTR("Apple Global Domain"))) {
      return *value;
    }
    // default value
    return true;
  }

  static boost::optional<uint8_t> get_keyboard_type(uint8_t country_code) {
    boost::optional<uint8_t> result;

    if (auto value = copy_dictionary_property(CFSTR("keyboardtype"), CFSTR("com.apple.keyboardtype"))) {
      uint16_t vendor_id = 0x16c0;
      uint16_t product_id = 0x27db;
      if (auto key = CFStringCreateWithCString(kCFAllocatorDefault,
                                               fmt::format("{0}-{1}-{2}",
                                                           static_cast<uint16_t>(product_id),
                                                           static_cast<uint16_t>(vendor_id),
                                                           country_code)
                                                   .c_str(),
                                               kCFStringEncodingUTF8)) {
        if (auto keyboard_type = CFDictionaryGetValue(value, key)) {
          result = cf_utility::to_int64_t(keyboard_type);
        }

        CFRelease(key);
      }

      CFRelease(value);
    }

    return result;
  }
};
} // namespace krbn
