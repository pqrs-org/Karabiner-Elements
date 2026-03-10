#pragma once

// `krbn::event_tap_utility` can be used safely in a multi-threaded environment.

#include "event_queue.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <optional>
#include <pqrs/osx/cg_event.hpp>

namespace krbn {
class event_tap_utility final {
public:
  static std::optional<event_queue::event> make_momentary_switch_event_from_key_code(CGEventRef event) {
    if (!event) {
      return std::nullopt;
    }

    auto key_code = pqrs::osx::cg_event::key_code::value_t(
        static_cast<uint16_t>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode)));

    if (auto usage_pair = pqrs::osx::cg_event::make_usage_pair(key_code)) {
      return event_queue::event(momentary_switch_event(*usage_pair));
    }

    return std::nullopt;
  }

  static std::optional<std::pair<event_type, event_queue::event>> make_event(CGEventType type,
                                                                             CGEventRef event) {
    switch (type) {
      case kCGEventKeyDown:
        if (auto e = make_momentary_switch_event_from_key_code(event)) {
          return std::make_pair(event_type::key_down, *e);
        }
        break;

      case kCGEventKeyUp:
        if (auto e = make_momentary_switch_event_from_key_code(event)) {
          return std::make_pair(event_type::key_up, *e);
        }
        break;

      case kCGEventFlagsChanged:
        if (auto e = make_momentary_switch_event_from_key_code(event)) {
          auto flags = CGEventGetFlags(event);
          auto event_type = get_modifier_event_type(*e, flags);
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

private:
  static std::optional<event_type> get_modifier_event_type(const event_queue::event& event,
                                                           CGEventFlags flags) {
    auto m = event.get_if<momentary_switch_event>();
    if (!m) {
      return std::nullopt;
    }

    auto usage_pair = m->get_usage_pair();
    auto usage_page = usage_pair.get_usage_page();
    auto usage = usage_pair.get_usage();

    if (usage_page == pqrs::hid::usage_page::keyboard_or_keypad) {
      if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift ||
          usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_right_shift) {
        return (flags & kCGEventFlagMaskShift) ? event_type::key_down : event_type::key_up;
      }
      if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_left_control ||
          usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_right_control) {
        return (flags & kCGEventFlagMaskControl) ? event_type::key_down : event_type::key_up;
      }
      if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_left_alt ||
          usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_right_alt) {
        return (flags & kCGEventFlagMaskAlternate) ? event_type::key_down : event_type::key_up;
      }
      if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui ||
          usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_right_gui) {
        return (flags & kCGEventFlagMaskCommand) ? event_type::key_down : event_type::key_up;
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
};
} // namespace krbn
