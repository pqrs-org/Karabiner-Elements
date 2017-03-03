#pragma once

#include "boost_defs.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <boost/optional.hpp>
#include <cmath>

namespace krbn {
class system_preferences final {
public:
  class values {
  public:
    values(void) : keyboard_fn_state_(system_preferences::get_keyboard_fn_state()) {
    }

    bool get_keyboard_fn_state(void) const { return keyboard_fn_state_; }

    bool operator==(const system_preferences::values& other) const {
      return keyboard_fn_state_ == other.keyboard_fn_state_;
    }
    bool operator!=(const system_preferences::values& other) const { return !(*this == other); }

  private:
    bool keyboard_fn_state_;
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

  /// "Use all F1, F2, etc. keys as standard function keys."
  static bool get_keyboard_fn_state(void) {
    if (auto value = system_preferences::get_bool_property(CFSTR("com.apple.keyboard.fnState"), CFSTR("Apple Global Domain"))) {
      return *value;
    }
    // default value
    return false;
  }
};
}
