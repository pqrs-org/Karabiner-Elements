#include "pressed_keys_manager.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "pressed_keys_manager"_test = [] {
    // empty

    {
      krbn::pressed_keys_manager manager;

      expect(manager.empty());
    }

    // key_code

    {
      krbn::pressed_keys_manager manager;

      manager.insert(krbn::momentary_switch_event(
          pqrs::hid::usage_page::keyboard_or_keypad,
          pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
      expect(!manager.empty());

      manager.erase(krbn::momentary_switch_event(
          pqrs::hid::usage_page::keyboard_or_keypad,
          pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
      expect(manager.empty());
    }

    // Duplicated key_code

    {
      krbn::pressed_keys_manager manager;

      manager.insert(krbn::momentary_switch_event(
          pqrs::hid::usage_page::keyboard_or_keypad,
          pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
      manager.insert(krbn::momentary_switch_event(
          pqrs::hid::usage_page::keyboard_or_keypad,
          pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
      expect(!manager.empty());

      manager.erase(krbn::momentary_switch_event(
          pqrs::hid::usage_page::keyboard_or_keypad,
          pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
      expect(manager.empty());
    }

    // consumer_key_code

    {
      krbn::pressed_keys_manager manager;

      manager.insert(krbn::momentary_switch_event(
          pqrs::hid::usage_page::consumer,
          pqrs::hid::usage::consumer::mute));
      expect(!manager.empty());

      manager.erase(krbn::momentary_switch_event(
          pqrs::hid::usage_page::consumer,
          pqrs::hid::usage::consumer::mute));
      expect(manager.empty());
    }

    // pointing_button

    {
      krbn::pressed_keys_manager manager;

      manager.insert(krbn::momentary_switch_event(
          pqrs::hid::usage_page::button,
          pqrs::hid::usage::button::button_1));
      expect(!manager.empty());

      manager.erase(krbn::momentary_switch_event(
          pqrs::hid::usage_page::button,
          pqrs::hid::usage::button::button_1));
      expect(manager.empty());
    }

    // combination

    {
      krbn::pressed_keys_manager manager;

      manager.insert(krbn::momentary_switch_event(
          pqrs::hid::usage_page::keyboard_or_keypad,
          pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
      expect(!manager.empty());

      manager.insert(krbn::momentary_switch_event(
          pqrs::hid::usage_page::keyboard_or_keypad,
          pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
      expect(!manager.empty());

      manager.insert(krbn::momentary_switch_event(
          pqrs::hid::usage_page::consumer,
          pqrs::hid::usage::consumer::mute));
      expect(!manager.empty());

      manager.insert(krbn::momentary_switch_event(
          pqrs::hid::usage_page::button,
          pqrs::hid::usage::button::button_1));
      expect(!manager.empty());

      manager.erase(krbn::momentary_switch_event(
          pqrs::hid::usage_page::keyboard_or_keypad,
          pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
      expect(!manager.empty());

      manager.erase(krbn::momentary_switch_event(
          pqrs::hid::usage_page::consumer,
          pqrs::hid::usage::consumer::mute));
      expect(!manager.empty());

      manager.erase(krbn::momentary_switch_event(
          pqrs::hid::usage_page::button,
          pqrs::hid::usage::button::button_10));
      expect(!manager.empty());

      manager.erase(krbn::momentary_switch_event(
          pqrs::hid::usage_page::button,
          pqrs::hid::usage::button::button_1));
      expect(manager.empty());
    }
  };

  return 0;
}
