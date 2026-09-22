#pragma once

// `krbn::event_tap_utility` can be used safely in a multi-threaded environment.

#include "event_queue.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <IOKit/hidsystem/IOLLEvent.h>
#include <optional>
#include <pqrs/osx/cg_event.hpp>

namespace krbn {
namespace event_tap_utility_details {
[[nodiscard]] inline std::optional<event_type> get_modifier_event_type(const event_queue::event& event,
                                                                       CGEventFlags flags) {
  auto m = event.get_if<momentary_switch_event>();
  if (!m) {
    return std::nullopt;
  }

  auto usage_pair = m->get_usage_pair();
  auto usage_page = usage_pair.get_usage_page();
  auto usage = usage_pair.get_usage();

  if (usage_page == pqrs::hid::usage_page::keyboard_or_keypad) {
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift) {
      return (flags & NX_DEVICELSHIFTKEYMASK) ? event_type::key_down : event_type::key_up;
    }
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_right_shift) {
      return (flags & NX_DEVICERSHIFTKEYMASK) ? event_type::key_down : event_type::key_up;
    }
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_left_control) {
      return (flags & NX_DEVICELCTLKEYMASK) ? event_type::key_down : event_type::key_up;
    }
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_right_control) {
      return (flags & NX_DEVICERCTLKEYMASK) ? event_type::key_down : event_type::key_up;
    }
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_left_alt) {
      return (flags & NX_DEVICELALTKEYMASK) ? event_type::key_down : event_type::key_up;
    }
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_right_alt) {
      return (flags & NX_DEVICERALTKEYMASK) ? event_type::key_down : event_type::key_up;
    }
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui) {
      return (flags & NX_DEVICELCMDKEYMASK) ? event_type::key_down : event_type::key_up;
    }
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_right_gui) {
      return (flags & NX_DEVICERCMDKEYMASK) ? event_type::key_down : event_type::key_up;
    }
    if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_caps_lock) {
      return (flags & kCGEventFlagMaskAlphaShift) ? event_type::key_down : event_type::key_up;
    }
  }

  if (usage_page == pqrs::hid::usage_page::apple_vendor_top_case &&
      usage == pqrs::hid::usage::apple_vendor_top_case::keyboard_fn) {
    return (flags & kCGEventFlagMaskSecondaryFn) ? event_type::key_down : event_type::key_up;
  }

  if (usage_page == pqrs::hid::usage_page::apple_vendor_keyboard &&
      usage == pqrs::hid::usage::apple_vendor_keyboard::function) {
    return (flags & kCGEventFlagMaskSecondaryFn) ? event_type::key_down : event_type::key_up;
  }

  return std::nullopt;
}
} // namespace event_tap_utility_details

class event_tap_utility final {
public:
  // Recover the virtual HID usage from the fixed CGEvent key-code-to-HID table.
  //
  // device_grabber::update_virtual_hid_keyboard configures our virtual keyboard
  // with vendor ID 0x05ac (Apple Aluminum USB Keyboard) and the product IDs below.
  // These mappings were verified in the macOS-supplied
  // AppleHIDKeyboard.kext/Contents/Info.plist (CFBundleVersion 9070.3):
  //
  // Type | Product ID | IOKit personality name              | alt_handler_id
  // -----|------------|-------------------------------------|---------------
  // ANSI | 0x024f     | Wired Keyboard 2007 B ANSI Map      | 46
  // ISO  | 0x0250     | Wired Keyboard 2007 B ISO Map       | 47
  // JIS  | 0x0251     | Wired Keyboard 2007 B JIS Map       | 48
  //
  // Apple's IOHIDPrivateKeys.h defines kgestM89ISOKbd = 47. IOHIDKeyboard::deviceType
  // uses alt_handler_id when _deviceType is not already set, so the ISO device matches
  // case kgestM89ISOKbd in its ISO-specific conversion:
  //   HID usage 0x35 (grave_accent_and_tilde) -> CGEvent key code 0x0a
  //   HID usage 0x64 (non_us_backslash)       -> CGEvent key code 0x32
  // The fixed CGEvent conversion table maps these codes back to the opposite HID
  // usages. Reverse that exchange for suppression and repeat matching.
  // ANSI and JIS use the normal mapping and need no exchange here.
  // https://github.com/apple-oss-distributions/IOHIDFamily/blob/777ccd9698845aadf711e32d843c8c9b777431d9/IOHIDFamily/IOHIDKeyboard.cpp#L388-L432
  // https://github.com/apple-oss-distributions/IOHIDFamily/blob/main/IOHIDFamily/IOHIDPrivateKeys.h#L93
  //
  // Use this copy only to match virtual HID output. If matching fails, preserve the
  // original event for fallback input and the loop guard: its source device is unknown.
  [[nodiscard]] static momentary_switch_event make_event_for_virtual_hid_matching(const momentary_switch_event& event,
                                                                                  bool virtual_hid_keyboard_is_iso) {
    if (virtual_hid_keyboard_is_iso &&
        event.get_usage_pair().get_usage_page() == pqrs::hid::usage_page::keyboard_or_keypad) {
      auto usage = event.get_usage_pair().get_usage();
      if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_grave_accent_and_tilde) {
        return momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                      pqrs::hid::usage::keyboard_or_keypad::keyboard_non_us_backslash);
      }
      if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_non_us_backslash) {
        return momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                      pqrs::hid::usage::keyboard_or_keypad::keyboard_grave_accent_and_tilde);
      }
    }
    return event;
  }

  [[nodiscard]] static std::optional<event_queue::event> make_momentary_switch_event(CGEventRef event) {
    if (!event) {
      return std::nullopt;
    }

    if (auto usage_pair = pqrs::osx::cg_event::make_usage_pair(event)) {
      return event_queue::event(momentary_switch_event(*usage_pair));
    }

    return std::nullopt;
  }

  [[nodiscard]] static std::optional<std::pair<event_type, event_queue::event>> make_event(CGEventType type,
                                                                                           CGEventRef event) {
    switch (type) {
      case kCGEventKeyDown:
        if (auto e = make_momentary_switch_event(event)) {
          return std::make_pair(event_type::key_down, *e);
        }
        break;

      case kCGEventKeyUp:
        if (auto e = make_momentary_switch_event(event)) {
          return std::make_pair(event_type::key_up, *e);
        }
        break;

      case static_cast<CGEventType>(NX_SYSDEFINED): {
        if (auto e = make_momentary_switch_event(event);
            e) {
          if (auto event_type = pqrs::osx::cg_event::make_event_type(event)) {
            switch (*event_type) {
              case pqrs::osx::cg_event::event_type::key_down:
                return std::make_pair(event_type::key_down, *e);

              case pqrs::osx::cg_event::event_type::key_up:
                return std::make_pair(event_type::key_up, *e);
            }
          }
        }
        break;
      }

      case kCGEventFlagsChanged:
        if (auto e = make_momentary_switch_event(event)) {
          auto flags = CGEventGetFlags(event);
          auto event_type = event_tap_utility_details::get_modifier_event_type(*e, flags);
          if (event_type) {
            return std::make_pair(*event_type, *e);
          }
        }
        break;

      case kCGEventLeftMouseDown:
        return std::make_pair(event_type::key_down,
                              event_queue::event(momentary_switch_event(pqrs::hid::usage_page::button,
                                                                        pqrs::hid::usage::button::button_1)));

      case kCGEventLeftMouseUp:
        return std::make_pair(event_type::key_up,
                              event_queue::event(momentary_switch_event(pqrs::hid::usage_page::button,
                                                                        pqrs::hid::usage::button::button_1)));

      case kCGEventRightMouseDown:
        return std::make_pair(event_type::key_down,
                              event_queue::event(momentary_switch_event(pqrs::hid::usage_page::button,
                                                                        pqrs::hid::usage::button::button_2)));

      case kCGEventRightMouseUp:
        return std::make_pair(event_type::key_up,
                              event_queue::event(momentary_switch_event(pqrs::hid::usage_page::button,
                                                                        pqrs::hid::usage::button::button_2)));

      case kCGEventOtherMouseDown:
        return std::make_pair(event_type::key_down,
                              event_queue::event(momentary_switch_event(pqrs::hid::usage_page::button,
                                                                        pqrs::hid::usage::button::button_3)));

      case kCGEventOtherMouseUp:
        return std::make_pair(event_type::key_up,
                              event_queue::event(momentary_switch_event(pqrs::hid::usage_page::button,
                                                                        pqrs::hid::usage::button::button_3)));

      case kCGEventMouseMoved:
      case kCGEventLeftMouseDragged:
      case kCGEventRightMouseDragged:
      case kCGEventOtherMouseDragged:
        return std::make_pair(event_type::single,
                              event_queue::event(pointing_motion()));

      case kCGEventScrollWheel: {
        // Set non-zero value for `manipulator::manipulators::base::unset_alone_if_needed`.
        pointing_motion pointing_motion;
        pointing_motion.set_vertical_wheel(1);
        return std::make_pair(event_type::single,
                              event_queue::event(pointing_motion));
      }

      case kCGEventNull:
      case kCGEventTabletPointer:
      case kCGEventTabletProximity:
      case kCGEventTapDisabledByTimeout:
      case kCGEventTapDisabledByUserInput:
        break;
    }

    return std::nullopt;
  }
};
} // namespace krbn
