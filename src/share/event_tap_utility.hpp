#pragma once

// `krbn::event_tap_utility` can be used safely in a multi-threaded environment.

#include "event_queue.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <atomic>
#include <optional>
#include <pqrs/osx/iokit_hid_system/key_code.hpp>

namespace krbn {
class event_tap_utility final {
public:
  static void update_latest_pointing_location(CGEventRef event) {
    if (!event) {
      return;
    }

    auto p = CGEventGetLocation(event);
    latest_pointing_location_x_.store(p.x, std::memory_order_relaxed);
    latest_pointing_location_y_.store(p.y, std::memory_order_relaxed);
    has_latest_pointing_location_.store(true, std::memory_order_relaxed);
  }

  static std::optional<CGPoint> get_latest_pointing_location(void) {
    if (!has_latest_pointing_location_.load(std::memory_order_relaxed)) {
      return std::nullopt;
    }

    return CGPointMake(latest_pointing_location_x_.load(std::memory_order_relaxed),
                       latest_pointing_location_y_.load(std::memory_order_relaxed));
  }

  static std::optional<event_queue::event> make_momentary_switch_event_from_key_code(CGEventRef event) {
    if (!event) {
      return std::nullopt;
    }

    auto key_code = pqrs::osx::iokit_hid_system::key_code::value_t(
        static_cast<uint16_t>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode)));

    if (auto usage = find_usage(key_code,
                                pqrs::osx::iokit_hid_system::key_code::impl::usage_page_keyboard_or_keypad_map)) {
      return event_queue::event(momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                       *usage));
    }

    if (auto usage = find_usage(key_code,
                                pqrs::osx::iokit_hid_system::key_code::impl::usage_page_apple_vendor_top_case_map)) {
      return event_queue::event(momentary_switch_event(pqrs::hid::usage_page::apple_vendor_top_case,
                                                       *usage));
    }

    if (auto usage = find_usage(key_code,
                                pqrs::osx::iokit_hid_system::key_code::impl::usage_page_apple_vendor_keyboard_map)) {
      return event_queue::event(momentary_switch_event(pqrs::hid::usage_page::apple_vendor_keyboard,
                                                       *usage));
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
        return make_pointing_button_event(event_type::key_down,
                                          event);

      case kCGEventOtherMouseUp:
        return make_pointing_button_event(event_type::key_up,
                                          event);

      case kCGEventMouseMoved:
      case kCGEventLeftMouseDragged:
      case kCGEventRightMouseDragged:
      case kCGEventOtherMouseDragged:
        return make_pointing_motion_event(event,
                                          false);

      case kCGEventScrollWheel: {
        return make_pointing_motion_event(event,
                                          true);
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
  inline static std::atomic<double> latest_pointing_location_x_{0.0};
  inline static std::atomic<double> latest_pointing_location_y_{0.0};
  inline static std::atomic<bool> has_latest_pointing_location_{false};

  template <typename Map>
  static std::optional<pqrs::hid::usage::value_t> find_usage(pqrs::osx::iokit_hid_system::key_code::value_t key_code,
                                                             const Map& map) {
    for (const auto& pair : map) {
      if (pair.second == key_code) {
        return pair.first;
      }
    }

    return std::nullopt;
  }

  static std::optional<std::pair<event_type, event_queue::event>> make_pointing_button_event(event_type event_type,
                                                                                             CGEventRef event) {
    if (!event) {
      return std::nullopt;
    }

    auto button_number = static_cast<int>(CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber)) + 1;
    button_number = std::clamp(button_number, 1, 32);

    auto usage = pqrs::hid::usage::value_t(type_safe::get(pqrs::hid::usage::button::button_1) +
                                           static_cast<uint32_t>(button_number - 1));

    return std::make_pair(event_type,
                          event_queue::event(momentary_switch_event(pqrs::hid::usage_page::button,
                                                                    usage)));
  }

  static std::optional<std::pair<event_type, event_queue::event>> make_pointing_motion_event(CGEventRef event,
                                                                                             bool scroll_wheel) {
    if (!event) {
      return std::nullopt;
    }

    auto x = static_cast<int>(CGEventGetIntegerValueField(event, kCGMouseEventDeltaX));
    auto y = static_cast<int>(CGEventGetIntegerValueField(event, kCGMouseEventDeltaY));
    auto vertical_wheel = static_cast<int>(CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1));
    auto horizontal_wheel = static_cast<int>(CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis2));

    if (scroll_wheel &&
        vertical_wheel == 0 &&
        horizontal_wheel == 0) {
      // Keep existing behavior for `unset_alone_if_needed` when wheel delta cannot be obtained.
      vertical_wheel = 1;
    }

    return std::make_pair(event_type::single,
                          event_queue::event(pointing_motion(x,
                                                             y,
                                                             vertical_wheel,
                                                             horizontal_wheel)));
  }

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
