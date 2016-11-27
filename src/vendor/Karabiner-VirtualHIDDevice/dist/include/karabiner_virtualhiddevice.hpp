// -*- Mode: c++ -*-

#pragma once

// Do not use <cstring> for kext
#include <stdint.h>
#include <string.h>

namespace pqrs {
class karabiner_virtualhiddevice final {
public:
  class hid_report final {
  public:
    class __attribute__((packed)) keyboard_input final {
    public:
      keyboard_input(void) : modifiers(0), reserved(0), keys{} {}
      bool operator==(const hid_report::keyboard_input& other) const { return (memcmp(this, &other, sizeof(*this)) == 0); }
      bool operator!=(const hid_report::keyboard_input& other) const { return !(*this == other); }

      uint8_t modifiers;
      uint8_t reserved;
      uint8_t keys[6];

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

  enum class event_type : uint32_t {
    key_down = 10,
    key_up = 11,
    flags_changed = 12,
  };

  class __attribute__((packed)) keyboard_event final {
  public:
    keyboard_event(void) : event_type(event_type::key_down),
                           flags(0),
                           key(0),
                           char_code(0),
                           char_set(0),
                           orig_char_code(0),
                           orig_char_set(0),
                           keyboard_type(0),
                           repeat(false) {}

    event_type event_type;
    uint32_t flags;
    uint32_t key;
    uint32_t char_code;
    uint32_t char_set;
    uint32_t orig_char_code;
    uint32_t orig_char_set;
    uint32_t keyboard_type;
    bool repeat;
  };

  class __attribute__((packed)) keyboard_special_event final {
  public:
    keyboard_special_event(void) : event_type(event_type::key_down),
                                   flags(0),
                                   key(0),
                                   flavor(0),
                                   guid(0),
                                   repeat(false) {}

    event_type event_type;
    uint32_t flags;
    uint32_t key;
    uint32_t flavor;
    uint64_t guid;
    bool repeat;
  };

  enum class user_client_method {
    // VirtualHIDKeyboard
    initialize_virtual_hid_keyboard,
    terminate_virtual_hid_keyboard,
    post_keyboard_input_report,
    reset_virtual_hid_keyboard,

    // VirtualHIDPointing
    initialize_virtual_hid_pointing,
    terminate_virtual_hid_pointing,
    post_pointing_input_report,
    reset_virtual_hid_pointing,

    // IOHIDSystem (since macOS 10.12)
    post_keyboard_event,
    post_keyboard_special_event,
    update_event_flags,

    end_,
  };

  static const char* get_virtual_hid_root_name(void) {
    return "org_pqrs_driver_Karabiner_VirtualHIDDevice_VirtualHIDRoot_v020800";
  }
};
}
