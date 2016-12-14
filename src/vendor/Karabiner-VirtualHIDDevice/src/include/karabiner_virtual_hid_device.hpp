// -*- Mode: c++ -*-

#pragma once

// Do not use <cstring> for kext
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
    av_top_case_keyboard_fn = 0x03,
    av_top_case_brightness_up = 0x04,
    av_top_case_brightness_down = 0x05,
    av_top_case_video_mirror = 0x06,
    av_top_case_illumination_toggle = 0x07,
    av_top_case_illumination_up = 0x08,
    av_top_case_illumination_down = 0x09,

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
    class __attribute__((packed)) keyboard_input final {
    public:
      keyboard_input(void) : report_id(1), modifiers(0), reserved(0), keys{}, apple_vendor_fn(0) {}
      bool operator==(const hid_report::keyboard_input& other) const { return (memcmp(this, &other, sizeof(*this)) == 0); }
      bool operator!=(const hid_report::keyboard_input& other) const { return !(*this == other); }

      uint8_t report_id;
      uint8_t modifiers;
      uint8_t reserved;
      uint8_t keys[6];
      uint8_t apple_vendor_fn;

      // modifiers:
      //   0x1 << 0 : left control
      //   0x1 << 1 : left shift
      //   0x1 << 2 : left option
      //   0x1 << 3 : left command
      //   0x1 << 4 : right control
      //   0x1 << 5 : right shift
      //   0x1 << 6 : right option
      //   0x1 << 7 : right command
    };

    class __attribute__((packed)) pointing_input final {
    public:
      pointing_input(void)
          : buttons{}, x(0), y(0), vertical_wheel(0), horizontal_wheel(0) {}

      uint8_t buttons[4]; // 32 bits for each button (32 buttons)
      uint8_t x;
      uint8_t y;
      uint8_t vertical_wheel;
      uint8_t horizontal_wheel;

      // buttons[0] = (0x1 << 0) -> button 1
      // buttons[0] = (0x1 << 1) -> button 2
      // buttons[0] = (0x1 << 2) -> button 3
      // ...
      // buttons[1] = (0x1 << 0) -> button 9
      // ...
      // buttons[2] = (0x1 << 0) -> button 17
      // ...
      // buttons[3] = (0x1 << 0) -> button 25
    };
  };

  class hid_event_service final {
  public:
    class __attribute__((packed)) keyboard_event final {
    public:
      keyboard_event(void) : usage_page(usage_page::keyboard_or_keypad) {}

      usage_page usage_page;
      usage usage;
      uint32_t value;
    };
  };

  enum class user_client_method {
    // VirtualHIDKeyboard
    initialize_virtual_hid_keyboard,
    terminate_virtual_hid_keyboard,
    dispatch_keyboard_event,
    post_keyboard_input_report,
    reset_virtual_hid_keyboard,

    // VirtualHIDPointing
    initialize_virtual_hid_pointing,
    terminate_virtual_hid_pointing,
    post_pointing_input_report,
    reset_virtual_hid_pointing,

    end_,
  };

  static const char* get_virtual_hid_root_name(void) {
    return "org_pqrs_driver_Karabiner_VirtualHIDDevice_VirtualHIDRoot_v030500";
  }
};
}
