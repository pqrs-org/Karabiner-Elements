#include "event_tap_utility.hpp"
#include <boost/ut.hpp>

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

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
