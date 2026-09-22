#include "event_tap_utility.hpp"
#include "keyboard_suppression.hpp"
#include "pressed_keys_manager.hpp"
#include <boost/ut.hpp>

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "virtual HID matching reverses ISO conversion without changing fallback input"_test = [] {
    namespace hid = pqrs::hid;
    using krbn::event_type;
    using krbn::momentary_switch_event;
    struct key final {
      hid::usage::value_t usage;
      CGKeyCode normal_code;
      CGKeyCode iso_code;
    };
    for (const auto* type : {"ansi", "iso", "jis"}) {
      for (const auto& key : {
               key{hid::usage::keyboard_or_keypad::keyboard_grave_accent_and_tilde, 0x32, 0x0a},
               key{hid::usage::keyboard_or_keypad::keyboard_non_us_backslash, 0x0a, 0x32},
               key{hid::usage::keyboard_or_keypad::keyboard_a, 0x00, 0x00},
           }) {
        bool iso = std::string_view(type) == "iso";
        momentary_switch_event posted(hid::usage_page::keyboard_or_keypad, key.usage);
        krbn::pressed_keys_manager pressed_keys;
        krbn::keyboard_suppression suppression;
        for (auto down : {true, false}) {
          // Use the same fixed key-code table as make_momentary_switch_event.
          auto usage_pair = pqrs::osx::cg_event::make_usage_pair(
              pqrs::osx::cg_event::key_code::value_t(iso ? key.iso_code : key.normal_code));
          expect(usage_pair.has_value()) << fatal;
          momentary_switch_event tapped(*usage_pair);
          auto original = tapped;
          auto et = down ? event_type::key_down : event_type::key_up;
          auto lookup = krbn::event_tap_utility::make_event_for_virtual_hid_matching(tapped, iso);
          expect(lookup == posted);
          expect(tapped == original);

          auto now = pqrs::osx::chrono::mach_absolute_time_point();
          // An unmatched lookup leaves the original fallback event intact.
          expect(!suppression.consume(lookup, et, now));
          suppression.enqueue(posted, et, now);
          expect(suppression.consume(lookup, et, now));
          expect(!suppression.consume(lookup, et, now));

          if (down) {
            pressed_keys.insert(posted);
          } else {
            pressed_keys.erase(posted);
          }
          expect(pressed_keys.contains(lookup) == down);
        }
      }
    }
    // The same numeric usage on another usage page must not be exchanged.
    momentary_switch_event other(hid::usage_page::consumer, hid::usage::value_t(0x35));
    expect(krbn::event_tap_utility::make_event_for_virtual_hid_matching(other, true) == other);
  };

  "make_event"_test = [] {
    {
      auto actual = krbn::event_tap_utility::make_event(kCGEventLeftMouseDown, nullptr);
      expect(actual->first == krbn::event_type::key_down);
      expect(actual->second == krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                                                     pqrs::hid::usage::button::button_1)));
    }
    {
      auto actual = krbn::event_tap_utility::make_event(kCGEventOtherMouseUp, nullptr);
      expect(actual->first == krbn::event_type::key_up);
      expect(actual->second == krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                                                     pqrs::hid::usage::button::button_3)));
    }
  };

  "make flags changed event for side-specific modifiers"_test = [] {
    struct modifier final {
      pqrs::hid::usage::value_t usage;
      CGEventFlags aggregate_mask;
      CGEventFlags own_mask;
      CGEventFlags opposite_mask;
    };

    for (const auto& modifier : {
             modifier{pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift, kCGEventFlagMaskShift, NX_DEVICELSHIFTKEYMASK, NX_DEVICERSHIFTKEYMASK},
             modifier{pqrs::hid::usage::keyboard_or_keypad::keyboard_right_shift, kCGEventFlagMaskShift, NX_DEVICERSHIFTKEYMASK, NX_DEVICELSHIFTKEYMASK},
             modifier{pqrs::hid::usage::keyboard_or_keypad::keyboard_left_control, kCGEventFlagMaskControl, NX_DEVICELCTLKEYMASK, NX_DEVICERCTLKEYMASK},
             modifier{pqrs::hid::usage::keyboard_or_keypad::keyboard_right_control, kCGEventFlagMaskControl, NX_DEVICERCTLKEYMASK, NX_DEVICELCTLKEYMASK},
             modifier{pqrs::hid::usage::keyboard_or_keypad::keyboard_left_alt, kCGEventFlagMaskAlternate, NX_DEVICELALTKEYMASK, NX_DEVICERALTKEYMASK},
             modifier{pqrs::hid::usage::keyboard_or_keypad::keyboard_right_alt, kCGEventFlagMaskAlternate, NX_DEVICERALTKEYMASK, NX_DEVICELALTKEYMASK},
             modifier{pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui, kCGEventFlagMaskCommand, NX_DEVICELCMDKEYMASK, NX_DEVICERCMDKEYMASK},
             modifier{pqrs::hid::usage::keyboard_or_keypad::keyboard_right_gui, kCGEventFlagMaskCommand, NX_DEVICERCMDKEYMASK, NX_DEVICELCMDKEYMASK},
         }) {
      auto event = krbn::event_queue::event(
          krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                       modifier.usage));

      auto down = krbn::event_tap_utility_details::get_modifier_event_type(
          event,
          modifier.aggregate_mask | modifier.own_mask | modifier.opposite_mask);
      expect(down == krbn::event_type::key_down);

      // The aggregate modifier remains set while the opposite-side key is held.
      // The released key must be classified using its side-specific flag.
      auto up = krbn::event_tap_utility_details::get_modifier_event_type(
          event,
          modifier.aggregate_mask | modifier.opposite_mask);
      expect(up == krbn::event_type::key_up);
    }
  };

  return 0;
}
