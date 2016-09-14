#pragma once

class system_preferences final {
public:
  class values {
  public:
    values(void) : keyboard_fn_state_(get_keyboard_fn_state()),
                   initial_key_repeat_milliseconds_(get_initial_key_repeat_milliseconds()),
                   key_repeat_milliseconds_(get_key_repeat_milliseconds()) {
    }

    bool get_keyboard_fn_state(void) const { return keyboard_fn_state_; }
    long get_initial_key_repeat_milliseconds(void) const { return initial_key_repeat_milliseconds_; }
    long get_key_repeat_milliseconds(void) const { return key_repeat_milliseconds_; }

    bool operator==(const system_preferences::values& other) const {
      return keyboard_fn_state_ == other.keyboard_fn_state_ &&
             initial_key_repeat_milliseconds_ == other.initial_key_repeat_milliseconds_ &&
             key_repeat_milliseconds_ == other.key_repeat_milliseconds_;
    }
    bool operator!=(const system_preferences::values& other) const { return !(*this == other); }

  private:
    bool keyboard_fn_state_;
    long initial_key_repeat_milliseconds_;
    long key_repeat_milliseconds_;
  };

  static bool get_long_property(CFStringRef _Nonnull key, CFStringRef _Nonnull application_id, long& value) {
    bool success = false;
    if (auto v = CFPreferencesCopyAppValue(key, application_id)) {
      success = CFNumberGetValue(static_cast<CFNumberRef>(v), kCFNumberLongType, &value);
      CFRelease(v);
    }
    return success;
  }

  /// "Use all F1, F2, etc. keys as standard function keys."
  static bool get_keyboard_fn_state(void) {
    long value;
    if (system_preferences::get_long_property(CFSTR("com.apple.keyboard.fnState"), CFSTR("Apple Global Domain"), value)) {
      return value;
    }
    // default value
    return false;
  }

  static long get_initial_key_repeat_milliseconds(void) {
    long value;
    if (system_preferences::get_long_property(CFSTR("InitialKeyRepeat"), CFSTR("Apple Global Domain"), value)) {
      // The unit of InitialKeyRepeat is 1/60 second.
      return value * 1000 / 60;
    }
    // default value
    return 500;
  }

  static long get_key_repeat_milliseconds(void) {
    long value;
    if (system_preferences::get_long_property(CFSTR("KeyRepeat"), CFSTR("Apple Global Domain"), value)) {
      // The unit of InitialKeyRepeat is 1/60 second.
      return value * 1000 / 60;
    }
    // default value
    return 83;
  }
};
