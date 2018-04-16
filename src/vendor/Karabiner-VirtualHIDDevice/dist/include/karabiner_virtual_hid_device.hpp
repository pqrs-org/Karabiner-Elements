// -*- Mode: c++ -*-

#pragma once

// Do not use <cstring> for kext
#include <IOKit/hid/IOHIDUsageTables.h>
#include <stdint.h>
#include <string.h>

namespace pqrs {
class karabiner_virtual_hid_device final {
public:
  enum class usage_page : uint32_t {
    generic_desktop = 0x01,
    keyboard_or_keypad = 0x07,
    consumer = 0x0c,

    // from AppleHIDUsageTables.h
    apple_vendor_top_case = 0xff,
    apple_vendor_keyboard = 0xff01,
  };

  enum class usage : uint32_t {
    gd_keyboard = 0x06,

    left_control = kHIDUsage_KeyboardLeftControl,
    left_shift = kHIDUsage_KeyboardLeftShift,
    left_option = kHIDUsage_KeyboardLeftAlt,
    left_command = kHIDUsage_KeyboardLeftGUI,
    right_control = kHIDUsage_KeyboardRightControl,
    right_shift = kHIDUsage_KeyboardRightShift,
    right_option = kHIDUsage_KeyboardRightAlt,
    right_command = kHIDUsage_KeyboardRightGUI,

    csmr_power = 0x30,
    csmr_display_brightness_increment = 0x6f,
    csmr_display_brightness_decrement = 0x70,
    csmr_fastforward = 0xb3,
    csmr_rewind = 0xb4,
    csmr_scan_next_track = 0xb5,
    csmr_scan_previous_track = 0xb6,
    csmr_eject = 0xb8,
    csmr_play_or_pause = 0xcd,
    csmr_mute = 0xe2,
    csmr_volume_increment = 0xe9,
    csmr_volume_decrement = 0xea,

    // from AppleHIDUsageTables.h
    apple_vendor_top_case_keyboard_fn = 0x03,
    apple_vendor_top_case_brightness_up = 0x04,
    apple_vendor_top_case_brightness_down = 0x05,
    apple_vendor_top_case_video_mirror = 0x06,
    apple_vendor_top_case_illumination_toggle = 0x07,
    apple_vendor_top_case_illumination_up = 0x08,
    apple_vendor_top_case_illumination_down = 0x09,

    apple_vendor_keyboard_spotlight = 0x01,
    apple_vendor_keyboard_dashboard = 0x02,
    apple_vendor_keyboard_launchpad = 0x04,
    apple_vendor_keyboard_expose_all = 0x10,
    apple_vendor_keyboard_expose_desktop = 0x11,
    apple_vendor_keyboard_brightness_up = 0x20,
    apple_vendor_keyboard_brightness_down = 0x21,
  };

  class hid_report final {
  public:
    enum class modifier : uint8_t {
      left_control = 0x1 << 0,
      left_shift = 0x1 << 1,
      left_option = 0x1 << 2,
      left_command = 0x1 << 3,
      right_control = 0x1 << 4,
      right_shift = 0x1 << 5,
      right_option = 0x1 << 6,
      right_command = 0x1 << 7,
    };

    class __attribute__((packed)) modifiers final {
    public:
      modifiers(void) : modifiers_(0) {}

      uint8_t get_raw_value(void) const {
        return modifiers_;
      }

      bool empty(void) const {
        return modifiers_ == 0;
      }

      void clear(void) {
        modifiers_ = 0;
      }

      void insert(modifier value) {
        modifiers_ |= static_cast<uint8_t>(value);
      }

      void erase(modifier value) {
        modifiers_ &= ~(static_cast<uint8_t>(value));
      }

      bool exists(modifier value) const {
        return modifiers_ & static_cast<uint8_t>(value);
      }

      bool operator==(const modifiers& other) const { return (memcmp(this, &other, sizeof(*this)) == 0); }
      bool operator!=(const modifiers& other) const { return !(*this == other); }

    private:
      uint8_t modifiers_;
    };

    class __attribute__((packed)) keys final {
    public:
      keys(void) : keys_{} {}

      const uint8_t (&get_raw_value(void) const)[32] {
        return keys_;
      }

      bool empty(void) const {
        for (const auto& k : keys_) {
          if (k != 0) {
            return false;
          }
        }
        return true;
      }

      void clear(void) {
        memset(keys_, 0, sizeof(keys_));
      }

      void insert(uint8_t key) {
        if (!exists(key)) {
          for (auto&& k : keys_) {
            if (k == 0) {
              k = key;
              return;
            }
          }
        }
      }

      void erase(uint8_t key) {
        for (auto&& k : keys_) {
          if (k == key) {
            k = 0;
          }
        }
      }

      bool exists(uint8_t key) const {
        for (const auto& k : keys_) {
          if (k == key) {
            return true;
          }
        }

        return false;
      }

      size_t count(void) const {
        size_t result = 0;
        for (const auto& k : keys_) {
          if (k) {
            ++result;
          }
        }
        return result;
      }

      bool operator==(const keys& other) const { return (memcmp(this, &other, sizeof(*this)) == 0); }
      bool operator!=(const keys& other) const { return !(*this == other); }

    private:
      uint8_t keys_[32];
    };

    class __attribute__((packed)) buttons final {
    public:
      buttons(void) : buttons_(0) {}

      uint32_t get_raw_value(void) const {
        return buttons_;
      }

      bool empty(void) const {
        return buttons_ == 0;
      }

      void clear(void) {
        buttons_ = 0;
      }

      void insert(uint8_t button) {
        if (1 <= button && button <= 32) {
          buttons_ |= (0x1 << (button - 1));
        }
      }

      void erase(uint8_t button) {
        if (1 <= button && button <= 32) {
          buttons_ &= ~(0x1 << (button - 1));
        }
      }

      bool exists(uint8_t button) const {
        if (1 <= button && button <= 32) {
          return buttons_ & (0x1 << (button - 1));
        }

        return false;
      }

      bool operator==(const buttons& other) const { return (memcmp(this, &other, sizeof(*this)) == 0); }
      bool operator!=(const buttons& other) const { return !(*this == other); }

    private:
      uint32_t buttons_; // 32 bits for each button (32 buttons)

      // (0x1 << 0) -> button 1
      // (0x1 << 1) -> button 2
      // (0x1 << 2) -> button 3
      // ...
      // (0x1 << 0) -> button 9
      // ...
      // (0x1 << 0) -> button 17
      // ...
      // (0x1 << 0) -> button 25
    };

    class __attribute__((packed)) keyboard_input final {
    public:
      keyboard_input(void) : report_id_(1), reserved(0) {}
      bool operator==(const keyboard_input& other) const { return (memcmp(this, &other, sizeof(*this)) == 0); }
      bool operator!=(const keyboard_input& other) const { return !(*this == other); }

    private:
      uint8_t report_id_ __attribute__((unused));

    public:
      modifiers modifiers;

    private:
      uint8_t reserved __attribute__((unused));

    public:
      keys keys;
    };

    class __attribute__((packed)) consumer_input final {
    public:
      consumer_input(void) : report_id_(2) {}
      bool operator==(const consumer_input& other) const { return (memcmp(this, &other, sizeof(*this)) == 0); }
      bool operator!=(const consumer_input& other) const { return !(*this == other); }

    private:
      uint8_t report_id_ __attribute__((unused));

    public:
      keys keys;
    };

    class __attribute__((packed)) apple_vendor_top_case_input final {
    public:
      apple_vendor_top_case_input(void) : report_id_(3) {}
      bool operator==(const apple_vendor_top_case_input& other) const { return (memcmp(this, &other, sizeof(*this)) == 0); }
      bool operator!=(const apple_vendor_top_case_input& other) const { return !(*this == other); }

    private:
      uint8_t report_id_ __attribute__((unused));

    public:
      keys keys;
    };

    class __attribute__((packed)) apple_vendor_keyboard_input final {
    public:
      apple_vendor_keyboard_input(void) : report_id_(4) {}
      bool operator==(const apple_vendor_keyboard_input& other) const { return (memcmp(this, &other, sizeof(*this)) == 0); }
      bool operator!=(const apple_vendor_keyboard_input& other) const { return !(*this == other); }

    private:
      uint8_t report_id_ __attribute__((unused));

    public:
      keys keys;
    };

    class __attribute__((packed)) pointing_input final {
    public:
      pointing_input(void) : buttons{}, x(0), y(0), vertical_wheel(0), horizontal_wheel(0) {}
      bool operator==(const pointing_input& other) const { return (memcmp(this, &other, sizeof(*this)) == 0); }
      bool operator!=(const pointing_input& other) const { return !(*this == other); }

      buttons buttons;
      uint8_t x;
      uint8_t y;
      uint8_t vertical_wheel;
      uint8_t horizontal_wheel;
    };
  };

  class properties final {
  public:
    class __attribute__((packed)) keyboard_initialization final {
    public:
      keyboard_initialization(void) : country_code(0) {}

      bool operator==(const keyboard_initialization& other) const {
        return country_code == other.country_code;
      }
      bool operator!=(const keyboard_initialization& other) const { return !(*this == other); }

      uint8_t country_code;
    };
  };

  enum class user_client_method {
    // VirtualHIDKeyboard
    initialize_virtual_hid_keyboard,
    terminate_virtual_hid_keyboard,
    is_virtual_hid_keyboard_ready,
    post_keyboard_input_report,
    post_consumer_input_report,
    post_apple_vendor_top_case_input_report,
    post_apple_vendor_keyboard_input_report,
    reset_virtual_hid_keyboard,

    // VirtualHIDPointing
    initialize_virtual_hid_pointing,
    terminate_virtual_hid_pointing,
    post_pointing_input_report,
    reset_virtual_hid_pointing,

    end_,
  };

  static const char* get_virtual_hid_root_name(void) {
    return "org_pqrs_driver_Karabiner_VirtualHIDDevice_VirtualHIDRoot_v060800";
  }

  static const char* get_kernel_extension_name(void) {
    return "org.pqrs.driver.Karabiner.VirtualHIDDevice.v060800.kext";
  }
};
} // namespace pqrs
