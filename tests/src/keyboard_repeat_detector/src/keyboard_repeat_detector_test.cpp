#include <catch2/catch.hpp>

#include "keyboard_repeat_detector.hpp"

TEST_CASE("is_repeating") {
  krbn::keyboard_repeat_detector keyboard_repeat_detector;
  REQUIRE(keyboard_repeat_detector.is_repeating() == false);

  keyboard_repeat_detector.set(pqrs::hid::usage_page::keyboard_or_keypad,
                               pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar,
                               krbn::event_type::key_up);
  REQUIRE(keyboard_repeat_detector.is_repeating() == false);

  // ----------------------------------------
  // Ignore modifier keys

  keyboard_repeat_detector.set(pqrs::hid::usage_page::keyboard_or_keypad,
                               pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar,
                               krbn::event_type::key_down);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(pqrs::hid::usage_page::keyboard_or_keypad,
                               pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift,
                               krbn::event_type::key_down);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  // ----------------------------------------
  // Cancel by key_up

  keyboard_repeat_detector.set(pqrs::hid::usage_page::keyboard_or_keypad,
                               pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar,
                               krbn::event_type::key_down);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(pqrs::hid::usage_page::keyboard_or_keypad,
                               pqrs::hid::usage::keyboard_or_keypad::keyboard_escape,
                               krbn::event_type::key_down);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(pqrs::hid::usage_page::keyboard_or_keypad,
                               pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift,
                               krbn::event_type::key_up);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(pqrs::hid::usage_page::keyboard_or_keypad,
                               pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar,
                               krbn::event_type::key_up);
  REQUIRE(keyboard_repeat_detector.is_repeating() == true);

  keyboard_repeat_detector.set(pqrs::hid::usage_page::keyboard_or_keypad,
                               pqrs::hid::usage::keyboard_or_keypad::keyboard_escape,
                               krbn::event_type::key_up);
  REQUIRE(keyboard_repeat_detector.is_repeating() == false);

  // ----------------------------------------
  // hid_value

  {
    pqrs::osx::iokit_hid_value hid_value(krbn::absolute_time_point(0),
                                         1,
                                         pqrs::hid::usage_page::keyboard_or_keypad,
                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar);
    keyboard_repeat_detector.set(hid_value);
    REQUIRE(keyboard_repeat_detector.is_repeating() == true);
  }
  {
    pqrs::osx::iokit_hid_value hid_value(krbn::absolute_time_point(0),
                                         0,
                                         pqrs::hid::usage_page::keyboard_or_keypad,
                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar);
    keyboard_repeat_detector.set(hid_value);
    REQUIRE(keyboard_repeat_detector.is_repeating() == false);
  }
}
